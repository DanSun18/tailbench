/** $lic$
 * Copyright (C) 2016-2017 by Massachusetts Institute of Technology
 *
 * This file is part of TailBench.
 *
 * If you use this software in your research, we request that you reference the
 * TaiBench paper ("TailBench: A Benchmark Suite and Evaluation Methodology for
 * Latency-Critical Applications", Kasture and Sanchez, IISWC-2016) as the
 * source in any publications that use this software, and that you send us a
 * citation of your work.
 *
 * TailBench is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "tbench_server.h"

#include <algorithm>
#include <atomic>
#include <vector>

#include "helpers.h"
#include "server.h"

#include <assert.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <sched.h> //for sched_getcpu


/*******************************************************************************
 * NetworkedServer
 *******************************************************************************/



NetworkedServer::NetworkedServer(int nthreads, std::string ip, int port, \
        int nclients) 
    : Server(nthreads)
{
    pthread_mutex_init(&sendLock, nullptr);
    pthread_mutex_init(&recvLock, nullptr);

    #ifdef CONTROL_WITH_QLEARNING //initialize conditional variable for q learning
    pthread_cond_init(&receiverCv,nullptr);
    #endif

    #ifdef PER_REQ_MONITOR
    pthread_mutex_init(&pcmLock, nullptr);
    #endif
    reqbuf = new Request[nthreads]; 

    activeFds.resize(nthreads);
    #ifdef PER_REQ_MONITOR
    cstates.resize(nthreads);
    sktstates.resize(nthreads);
    #endif
    recvClientHead = 0;

    #ifdef CONTROL_WITH_QLEARNING //initialize starttime variable
    starttime = 0; //when is starttime used? waht is starttime?
    current_window_id = 0;
    #endif
    // Get address info
    int status;
    struct addrinfo hints;
    struct addrinfo* servInfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    std::stringstream portstr;
    portstr << port;

    const char* ipstr = (ip.size() > 0) ? ip.c_str() : nullptr;

    if ((status = getaddrinfo(ipstr, portstr.str().c_str(), &hints, &servInfo))\
            != 0) {
        std::cerr << "getaddrinfo() failed: " << gai_strerror(status) \
            << std::endl;
        exit(-1);
    }

    // Create listening socket
    int listener = socket(servInfo->ai_family, servInfo->ai_socktype, \
            servInfo->ai_protocol);
    if (listener == -1) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        exit(-1);
    }

    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) \
            == -1)  {
        std::cerr << "setsockopt() failed: " << strerror(errno) << std::endl;
        exit(-1);
    }

    if (bind(listener, servInfo->ai_addr, servInfo->ai_addrlen) == -1) {
        std::cerr << "bind() failed: " << strerror(errno) << std::endl;
        exit(-1);
    }

    if (listen(listener, 10) == -1) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        exit(-1);
    }

    // Establish connections with clients
    struct sockaddr_storage clientAddr;
    socklen_t clientAddrSize;

    for (int c = 0; c < nclients; ++c) {
        clientAddrSize = sizeof(clientAddr);
        memset(&clientAddr, 0, clientAddrSize);

        int clientFd = accept(listener, \
                reinterpret_cast<struct sockaddr*>(&clientAddr), \
                &clientAddrSize);

        if (clientFd == -1) {
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
            exit(-1);
        }

        int nodelay = 1;
        if (setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, 
                reinterpret_cast<char*>(&nodelay), sizeof(nodelay)) == -1) {
            std::cerr << "setsockopt(TCP_NODELAY) failed: " << strerror(errno) \
                << std::endl;
            exit(-1);
        }

        clientFds.push_back(clientFd);
        #ifdef CONTROL_WITH_QLEARNING //initialize shared memory for Q Learning
        init_shm();
        #endif
    }
}

NetworkedServer::~NetworkedServer() {
    delete reqbuf;
}

void NetworkedServer::removeClient(int fd) {
    auto it = std::find(clientFds.begin(), clientFds.end(), fd);
    clientFds.erase(it);
}

bool NetworkedServer::checkRecv(int recvd, int expected, int fd) {
    bool success = false;
    if (recvd == 0) { // Client exited
        std::cerr << "Client left, removing" << std::endl;
        removeClient(fd);
        success = false;
    } else if (recvd == -1) {
        std::cerr << "recv() failed: " << strerror(errno) \
            << ". Exiting" << std::endl;
        exit(-1);
    } else {
        if (recvd != expected) {
            std::cerr << "ERROR! recvd = " << recvd << ", expected = " \
                << expected << std::endl;
            exit(-1);
        }
        // assert(recvd == expected);
        success = true;
    }

    return success;
}

/*
    Method in the loop body of the receiver thread
    return false if there is nothing in the clientFd, true otherwise
*/

bool NetworkedServer::putReqInQueue() {  

    bool success = false;
    Request *req = new Request;
    int fd = -1;
//    std::cerr << "server wants to recv req" << std::endl;
    while(!success && clientFds.size() > 0) { //read request from socket
        int maxFd = -1;
        fd_set readSet;
        FD_ZERO(&readSet);
        for (int f : clientFds) {
            FD_SET(f, &readSet);
            if (f > maxFd) maxFd = f;
        }

        //wait for one of the file descriptor get ready
        int ret = select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
        if (ret == -1) {
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            exit(-1);
        }

        fd = -1;

        for (size_t i = 0; i < clientFds.size(); ++i) {
            size_t idx = (recvClientHead + i) % clientFds.size();
            if (FD_ISSET(clientFds[idx], &readSet)) {
                fd = clientFds[idx];
                break;
            }

        }

        recvClientHead = (recvClientHead + 1) % clientFds.size();

        assert(fd != -1);

        //process header first
        int header_len = sizeof(Request) - MAX_REQ_BYTES;
        int recvd = recvfull(fd, reinterpret_cast<char*>(req), header_len, 0);
        success = checkRecv(recvd, header_len, fd);

        if (!success) continue;

        recvd = recvfull(fd, req->data, req->len, 0);

        success = checkRecv(recvd, req->len, fd);
//	std::cerr << "receive success mark is " << success << std::endl;
        if (!success) continue;

    }
   
    pthread_mutex_lock(&recvLock);
    reqQueue.push(req);
    unsigned int reqQueueLength = reqQueue.size();
//  std::cerr << "receive req.." << reqQueue.size() << std::endl;
    fds.push(fd);
    reqQueueLengths.push_back(reqQueueLength);
    reqReceivingTimes.push(getCurNs());
    pthread_cond_signal(&receiverCv);
    pthread_mutex_unlock(&recvLock);

    return clientFds.size() > 0;
}

//arguments: id is thread id
#ifdef CONTROL_WITH_QLEARNING //determine which recvReq() method to use
size_t NetworkedServer::recvReq(int id, void** data) {

  //  std::cerr << "reach here 1 " <<std::endl;
    pthread_mutex_lock(&recvLock);
  //  std::cerr << "reach here 2 " << reqQueue.empty()<<std::endl;
    while(reqQueue.empty())
    {
//	std::cerr << reqQueue.size() << std::endl;
	 pthread_cond_wait(&receiverCv,&recvLock);
    }
 //   std::cerr << "reach here 3 " << std::endl;
    Request *req = reqQueue.front();
    reqQueue.pop();
   // std::cerr << "recv req "<<req->id << std::endl;    
    int fd = fds.front();
    fds.pop();
    int QL = reqQueueLengths.front();
    reqQueueLengths.pop_front();
    uint64_t rectime = reqReceivingTimes.front();
    reqReceivingTimes.pop();
    uint64_t curNs = getCurNs();
    reqInfo[id].id = req->id;
    reqInfo[id].startNs = curNs;
    reqInfo[id].Qlength = QL;
    reqInfo[id].RecNs = rectime;
    activeFds[id] = fd;
    *data = reinterpret_cast<void*>(req->data);
    size_t len = req->len;
    //reqInfo[id].reqlen = len;
    global_req = req;
    pthread_mutex_unlock(&recvLock);
    return len;
    
}
#else
size_t NetworkedServer::recvReq(int id, void** data) {
    pthread_mutex_lock(&recvLock);

    bool success = false;
    Request* req;
    int fd = -1;
    while (!success && clientFds.size() > 0) {
        int maxFd = -1;
        fd_set readSet;
        FD_ZERO(&readSet);
        for (int f : clientFds) {
            FD_SET(f, &readSet);
            if (f > maxFd) maxFd = f;
        }
	
        int ret = select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
        if (ret == -1) {
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            exit(-1);
        }
        fd = -1;
        for (size_t i = 0; i < clientFds.size(); ++i) {
            size_t idx = (recvClientHead + i) % clientFds.size();
            if (FD_ISSET(clientFds[idx], &readSet)) {
                fd = clientFds[idx];
                break;
            }
        }

        recvClientHead = (recvClientHead + 1) % clientFds.size();

        assert(fd != -1);

        int len = sizeof(Request) - MAX_REQ_BYTES; // Read request header first

        req = &reqbuf[id];
	
	//std::cerr<<"begin recv full...."<<std::endl;
        int recvd = recvfull(fd, reinterpret_cast<char*>(req), len, 0);
       // std::cerr<<"end recv full...."<<std::endl;
        success = checkRecv(recvd, len, fd);
	//std::cerr << "first check "<<success << std::endl;
        if (!success) continue;
        
        recvd = recvfull(fd, req->data, req->len, 0);
        //std::cerr << "second check "<<success << std::endl;
        success = checkRecv(recvd, req->len, fd);
        if (!success) continue;
    }
     //std::cerr<<"finish retreive request from client port..."<<std::endl;

    if (clientFds.size() == 0) {
        std::cerr << "All clients exited. Server finishing" << std::endl;
        exit(0);
    } else {
        
        
        #ifdef PER_REQ_MONITOR
        uint64_t arrvNs = getCurNs();
        // When start to process each request, note down counter states with performance counter
        //find out core id current thread is on
        
        unsigned int coreId = sched_getcpu();
        unsigned int socketID = 0; //it would be better for the thread to figure this out too
                            //but now using constant assuming it's always going to be 1
        //for debugging why instr and L3Miss sometimes ~= 2^64
        // std::cout << std::string("Thread ") << id << std::string(": received request ") << req->id << '\n';
        // std::cout << "\toperating on core " << coreId << '\n';

        pthread_mutex_lock(&pcmLock);
        CoreCounterState core_state = pcm->getCoreCounterState(coreId);
        SocketCounterState socket_state = pcm->getSocketCounterState(socketID);
        pthread_mutex_unlock(&pcmLock);
        cstates[id] = core_state;
        sktstates[id] = socket_state;
        #endif

        *data = reinterpret_cast<void*>(&req->data);

        
        uint64_t startNs = getCurNs();
        reqInfo[id].id = req->id;
        reqInfo[id].startNs = startNs;
        activeFds[id] = fd;

        #ifdef PER_REQ_MONITOR
        reqInfo[id].arrvNs = arrvNs;
        #endif
        
    }

    pthread_mutex_unlock(&recvLock);

    return req->len;
};
#endif


void NetworkedServer::sendResp(int id, const void* data, size_t len) {
    pthread_mutex_lock(&sendLock);

    Response* resp = new Response();
    
    #ifdef CONTROL_WITH_QLEARNING // set starttime when server sends the first response out
    if (starttime == 0)
        starttime = getCurNs();
    #endif

    resp->type = RESPONSE;
    resp->id = reqInfo[id].id;
    resp->len = len;
    #ifdef CONTROL_WITH_QLEARNING //write queue length into response to send to client
    resp->queue_len = reqInfo[id].Qlength;
    #endif
    memcpy(reinterpret_cast<void*>(&resp->data), data, len);

    

    uint64_t svcFinishNs = getCurNs();
    assert(svcFinishNs > reqInfo[id].startNs);
    resp->svcNs = svcFinishNs - reqInfo[id].startNs;
    resp->startNs = reqInfo[id].startNs;


    #ifdef PER_REQ_MONITOR
    // finishing up request, find counter parameters
      //find out core id current thread is on
    unsigned int coreId = sched_getcpu();
    unsigned int socketID = 0; //it would be better for the thread to figure this out too
                           //but now using constant assuming it's always going to be 1

    //for debugging why instr and L3Miss sometimes ~= 2^64
    // std::cout << "Thread " << id << ": sending response " << resp->id << '\n';
    // std::cout << "\toperating on core " <<  coreId << '\n';


    pthread_mutex_lock(&pcmLock);
    CoreCounterState core_state = pcm->getCoreCounterState(coreId);
    SocketCounterState socket_state = pcm->getSocketCounterState(socketID);
    pthread_mutex_unlock(&pcmLock);

    //unsigned long int instrBefore = getInstructionsRetired(cstates[id]);
    // std::cout << "\tNumber of instructions on core counter before processing:" <<  instrBefore << '\n';
    //unsigned long int instrAfter = getInstructionsRetired(core_state);
    // std::cout << "\tNumber of instructions on core counter after processing:" <<   instrAfter << '\n';

    unsigned long int instr = getInstructionsRetired(cstates[id], core_state);
    unsigned long int bytesRead = getBytesReadFromMC(sktstates[id], socket_state);
    unsigned long int bytesWritten = getBytesWrittenToMC(sktstates[id], socket_state);
    unsigned long int L3Miss = getL3CacheMisses(cstates[id], core_state);
    double L3HitRatio = getL3CacheHitRatio(cstates[id], core_state);
    unsigned long int L3Occupancy = getL3CacheOccupancy(core_state);
  
    resp->coreId = coreId;
    resp->instr = instr;
    resp->bytesRead = bytesRead;
    resp->bytesWritten = bytesWritten;
    resp->L3MissNum = L3Miss;
    resp->L3Occupancy = L3Occupancy;
    resp->L3HitRate = L3HitRatio;

    uint64_t depatureNs = getCurNs();
    assert(depatureNs > reqInfo[id].arrvNs);
    resp->serverNs = depatureNs - reqInfo[id].arrvNs;
    resp->arrvNs = reqInfo[id].arrvNs;
    #endif

    int fd = activeFds[id];
    int totalLen = sizeof(Response) - MAX_RESP_BYTES + len;
    int sent = sendfull(fd, reinterpret_cast<const char*>(resp), totalLen, 0);
    //std::cerr << "length is " <<sent <<std::endl;
    assert(sent == totalLen);

    ++finishedReqs;
    //std::cerr << finishedReqs<<' '<<warmupReqs<<std::endl;
    if (finishedReqs == warmupReqs) {
        resp->type = ROI_BEGIN;
        for (int fd : clientFds) {
            totalLen = sizeof(Response) - MAX_RESP_BYTES;
            sent = sendfull(fd, reinterpret_cast<const char*>(resp), totalLen, 0);
            assert(sent == totalLen);
        }
    } else if (finishedReqs == warmupReqs + maxReqs) { 
        resp->type = FINISH;
        for (int fd : clientFds) {
            totalLen = sizeof(Response) - MAX_RESP_BYTES;
            sent = sendfull(fd, reinterpret_cast<const char*>(resp), totalLen, 0);
            assert(sent == totalLen);
        }
    }
    
    #ifdef CONTROL_WITH_QLEARNING //upate info in shared memory when responsed is sent
    //std::cerr << "resp request " << std::endl;
    latencies.push_back(svcFinishNs-reqInfo[id].RecNs);
    services.push_back(resp->svcNs);
    update_mem();
    #endif

    delete resp;
    
    pthread_mutex_unlock(&sendLock);
}

void NetworkedServer::finish() {
    pthread_mutex_lock(&sendLock);

    Response* resp = new Response();
    resp->type = FINISH;

    for (int fd : clientFds) {
        int len = sizeof(Response) - MAX_RESP_BYTES;
        int sent = sendfull(fd, reinterpret_cast<const char*>(resp), len, 0);
        assert(sent == len);
    }

    delete resp;
    
    pthread_mutex_unlock(&sendLock);
}

#ifdef CONTROL_WITH_QLEARNING //methods for server running with Q Learning
void NetworkedServer::init_shm()
{   
    const int SERVER_INFO_SIZE = sizeof(double);

    const int STATE_INFO_SIZE = sizeof(state_info_t);

    state_info_fd = shm_open(state_info_shm_file_name, O_CREAT | O_RDWR, 0666);
    if(state_info_fd == -1) {
        printf("Failed to crate shared memory for state_info: %s\n", strerror(errno));
        exit(1);
    }


    server_info_fd = shm_open(server_info_shm_file_name, O_CREAT | O_RDWR, 0666);
    if (server_info_fd == -1) {
        printf("Failed to crate shared memory for server_info: %s\n", strerror(errno));
        exit(1);
    }

    //sem_fd = shm_open(sem_name,O_CREAT|O_RDWR,0666);
    
    ftruncate(server_info_fd, SERVER_INFO_SIZE);
    ftruncate(state_info_fd,STATE_INFO_SIZE);

    server_info_mem_addr = mmap(NULL, SERVER_INFO_SIZE, 
        PROT_READ | PROT_WRITE, MAP_SHARED, server_info_fd, 0);
    state_info_mem_addr = mmap(NULL, STATE_INFO_SIZE, 
        PROT_READ | PROT_WRITE, MAP_SHARED, state_info_fd, 0);
}


void NetworkedServer::update_mem()
{
    unsigned int curtime = getCurNs();
    if(curtime - starttime < 100e6) return;

    
    //get 95th percentile latency in ms (round up)
    std::sort(latencies.begin(),latencies.end());
    unsigned int len = latencies.size();
    unsigned int index = (unsigned int) ceil(len*0.95);
    double latency_in_ms = (double)(latencies[index]/1000000.0);

    // get 95 th percentile service time in ms (round up)
    std::sort(services.begin(),services.end());
    len = services.size();
    index = (unsigned int) ceil(len*0.95);
    double max_service_time_in_ms = (double)(services[index]/1000000.0);

    // get maximum of queue length when the request arrives for requests currently in the queue
    std::deque<unsigned int>::iterator max_queue_ptr = std::max_element(
        reqQueueLengths.begin(), reqQueueLengths.end());
    unsigned int max_QL = *max_queue_ptr; //current length of the queue

    
    //std::cerr << val << std::endl;
    //print for debugging
    // std::cout << "update_mem(): " << "window_id = " << current_window_id << "\n"
    //     << "\t" << "Qlength = " << max_QL << "\n"
    //     << "\t" << "service_time = " << max_service_time_in_ms << "\n"
    //     << "\t" << "latency = " << latency_in_ms << "\n";  

    update_server_info(max_QL,max_service_time_in_ms);
    memcpy(server_info_mem_addr, &latency_in_ms,sizeof(double));

    latencies.clear();
    services.clear();
    starttime = getCurNs();
}


void NetworkedServer::update_server_info(unsigned int Qlength, float service_time)
{
    unsigned int this_window_id = current_window_id;
    current_window_id++;
    state_info.window_id = this_window_id;
    state_info.Qlength = Qlength;
    state_info.service_time = service_time;
    memcpy(state_info_mem_addr,&state_info,sizeof(state_info_t));
}

#endif
/*******************************************************************************
 * Per-thread State
 *******************************************************************************/
__thread int tid;

/*******************************************************************************
 * Global data
 *******************************************************************************/
std::atomic_int curTid;
NetworkedServer* server;
pthread_mutex_t threadCreateLock;
//for receiver thread
pthread_t* receiverThread;

Request *global_req;
pthread_cond_t receiverCv;


/*******************************************************************************
 * API
 *******************************************************************************/
void tBenchServerInit(int nthreads) {
    
   	pthread_mutex_init(&threadCreateLock, nullptr); //initate lock for creating threads

    //set core for meta-thread (current thread) 
    //parse envrionmental variable, if given   
    cpu_set_t metaThreadCpuSet;  
    CPU_ZERO(&metaThreadCpuSet);
    int meta_thread_core = getOpt<int>("META_THREAD_CORE", 7);
    CPU_SET(meta_thread_core, &metaThreadCpuSet);
    //set current thread to core according to environment variable
    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &metaThreadCpuSet) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    } else {
        std::cerr << "Sucessfully set thread " << thread << " on core " << meta_thread_core << "\n";
    }
    //reporting meta-thread affinity to user
    unsigned int coreId = sched_getcpu();
    std::cout << "Confirmation: Meta thread running on core " << coreId << '\n';

    #ifdef PER_REQ_MONITOR //If the user wants per request monitor, start the pcm
        std::cout << "----------PCM Starting----------" << '\n'; 
        pcm = PCM::getInstance();
        pcm->resetPMU();
        PCM::ErrorCode status = pcm->program();
        switch (status)
        {
        case PCM::Success:
            break;
        case PCM::MSRAccessDenied:
            std::cerr << "Access to Intel(r) Performance Counter Monitor has denied (no MSR or PCI CFG space access)." << std::endl;
            exit(EXIT_FAILURE);
        case PCM::PMUBusy:
            std::cerr << "Access to Intel(r) Performance Counter Monitor has denied (Performance Monitoring Unit is occupied by other application). Try to stop the application that uses PMU." << std::endl;
            std::cerr << "Alternatively you can try running Intel PCM with option -r to reset PMU configuration at your own risk." << std::endl;
            exit(EXIT_FAILURE);
        default:
            std::cerr << "Access to Intel(r) Performance Counter Monitor has denied (Unknown error)." << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cerr << "\nDetected " << pcm->getCPUBrandString() << " \"Intel(r) microarchitecture codename " << pcm->getUArchCodename() << "\"" << std::endl;
        
        // const int cpu_model = pcm->getCPUModel();
        std::cout << "---------PCM Started----------" << '\n';
    #endif //PER_REQ_MONITOR

    std::cout << "----------Server Starting----------" << '\n';
    curTid = 0;
    std::string serverurl = getOpt<std::string>("TBENCH_SERVER", "");
    int serverport = getOpt<int>("TBENCH_SERVER_PORT", 7000);
    int nclients = getOpt<int>("TBENCH_NCLIENTS", 1);
    // std::cout << "TESTING: " << nthreads << " threads for server are detected\n";
    server = new NetworkedServer(nthreads, serverurl, serverport, nclients);
    std::cout << "----------Server Started----------" << '\n';
    setupReceiverThread();
    std::cout << "----------Receiver thread set up----------" << '\n';
}

void tBenchServerThreadStart() {
    pthread_mutex_lock(&threadCreateLock);
    tid = curTid++;
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);
    std::string parsing_text;
    parsing_text = "SERVER_THREAD_" + std::to_string(tid) + "_CORE";
    int server_thread_core = getOpt<int>(parsing_text.c_str(), 2);
    CPU_SET(server_thread_core, &thread_cpu_set);
    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &thread_cpu_set) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    } else {
        std::cerr << "Sucessfully set thread " << thread << " on core " << server_thread_core << "\n";
    }
    pthread_mutex_unlock(&threadCreateLock);
}

void tBenchServerFinish() {
    #ifdef PER_REQ_MONITOR
	pcm->cleanup();
    #endif
    server->finish();
}

size_t tBenchRecvReq(void** data) {
    return server->recvReq(tid, data);
}

void tBenchSendResp(const void* data, size_t size) {
    return server->sendResp(tid, data, size);
}


/*******************************************************************************
 * Receiver thread 
 *******************************************************************************/


void* receiverThreadFunc(void *ptr)
{   
    NetworkedServer *server = (NetworkedServer*)ptr;
    bool fdAvailable = true;

    while(fdAvailable) 
    {
        fdAvailable = server->putReqInQueue();
    }

    return 0;
}

void setupReceiverThread()
{
    //set up receiver thread object
    receiverThread = new pthread_t;
    //parse intended receiver thread core from environment variable
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int receiverThreadCore = getOpt<int>("RECEIVER_THREAD_CORE", 6);
    CPU_SET(receiverThreadCore,&cpuset);
    //intialize attr object with intended thread affinity
    pthread_attr_t *attr;
    attr = new pthread_attr_t;
    pthread_attr_init(attr);
    pthread_attr_setaffinity_np(attr,sizeof(cpu_set_t),&cpuset);
    //create the receiver thread
    pthread_create(receiverThread, attr, receiverThreadFunc, (void *)server);
  
}


void tBench_deleteReq() 
{
    delete global_req;
}

#ifdef CONTROL_WITH_QLEARNING //implementaion of necessary functions for receiver thread
void tBench_join() 
{   
    pthread_join(*receiverThread,NULL);
}

#else
void tBench_join(){
    //do nothing
}
#endif
