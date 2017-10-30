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
 #ifdef CONTROL_WITH_QLEARNING //addtional variables for Q Learning
 std::queue<Request> get_Queue() { //posibily never used?
    std::queue<Request> Q;
    return Q;
 }

pthread_t* receive_thread;
pthread_attr_t *attr;
Request *global_req;

pthread_cond_t rec_cv;
#endif

NetworkedServer::NetworkedServer(int nthreads, std::string ip, int port, \
        int nclients) 
    : Server(nthreads)
{
    pthread_mutex_init(&sendLock, nullptr);
    pthread_mutex_init(&recvLock, nullptr);

    #ifdef CONTROL_WITH_QLEARNING //initialize conditional variable for q learning
    pthread_cond_init(&rec_cv,nullptr);
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


#ifdef CONTROL_WITH_QLEARNING // method for the receiver thread
int NetworkedServer::recvReq_Q() {

    bool success = false;
    Request *req = new Request;
    int fd = -1;
//    std::cerr << "server wants to recv req" << std::endl;
    while(!success && clientFds.size() > 0) {
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

        //process head first
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
    recvReq_Queue.push(req);
    unsigned int Qlen = recvReq_Queue.size();
//    std::cerr << "receive req.." << recvReq_Queue.size() << std::endl;
    fd_Queue.push(fd);
    Qlen_Queue.push_back(Qlen);
    rectime_Queue.push(getCurNs());
    pthread_cond_signal(&rec_cv);
    pthread_mutex_unlock(&recvLock);
    if (clientFds.size() == 0) {
        return 0;
    }
    else {
        return 1;
    }

}

#endif

//arguments: id is thread id
#ifdef CONTROL_WITH_QLEARNING //determine which recvReq() method to use
size_t NetworkedServer::recvReq(int id, void** data) {

  //  std::cerr << "reach here 1 " <<std::endl;
    pthread_mutex_lock(&recvLock);
  //  std::cerr << "reach here 2 " << recvReq_Queue.empty()<<std::endl;
    while(recvReq_Queue.empty())
    {
//	std::cerr << recvReq_Queue.size() << std::endl;
	 pthread_cond_wait(&rec_cv,&recvLock);
    }
 //   std::cerr << "reach here 3 " << std::endl;
    Request *req = recvReq_Queue.front();
    recvReq_Queue.pop();
   // std::cerr << "recv req "<<req->id << std::endl;    
    int fd = fd_Queue.front();
    fd_Queue.pop();
    int QL = Qlen_Queue.front();
    Qlen_Queue.pop_front();
    uint64_t rectime = rectime_Queue.front();
    rectime_Queue.pop();
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
        
        unsigned int coreID = sched_getcpu();
        unsigned int socketID = 0; //it would be better for the thread to figure this out too
                            //but now using constant assuming it's always going to be 1
        //for debugging why instr and L3Miss sometimes ~= 2^64
        // std::cout << std::string("Thread ") << id << std::string(": received request ") << req->id << '\n';
        // std::cout << "\toperating on core " << coreID << '\n';

        pthread_mutex_lock(&pcmLock);
        CoreCounterState core_state = pcm->getCoreCounterState(coreID);
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
    unsigned int coreID = sched_getcpu();
    unsigned int socketID = 0; //it would be better for the thread to figure this out too
                           //but now using constant assuming it's always going to be 1

    //for debugging why instr and L3Miss sometimes ~= 2^64
    // std::cout << "Thread " << id << ": sending response " << resp->id << '\n';
    // std::cout << "\toperating on core " <<  coreID << '\n';


    pthread_mutex_lock(&pcmLock);
    CoreCounterState core_state = pcm->getCoreCounterState(coreID);
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

    
    resp->coreId = coreID;
    resp->instr = instr;
    resp->bytesRead = bytesRead;
    resp->bytesWritten = bytesWritten;
    resp->L3MissNum = L3Miss;
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
    //TODO: change to only update memory every 50ms 
    //to reduce overhead?
    unsigned int curtime = getCurNs();
    if(curtime - starttime < 5e7) return;

    
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
        recvReq_Queue.begin(), recvReq_Queue.end());
    unsigned int max_QL = *max_queue_ptr; //current length of the queue

    
    //std::cerr << val << std::endl;
    //print for debugging
    std::cout << "update_mem(): " << "window_id = " << current_window_id << "\n"
        << "\t" << "Qlength = " << max_QL << "\n"
        << "\t" << "service_time = " << max_service_time_in_ms << "\n"
        << "\t" << "latency = " << latency_in_ms << "\n";  

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
//__thread int coreId;

/*******************************************************************************
 * Global data
 *******************************************************************************/
std::atomic_int curTid;
NetworkedServer* server;
// cpu_set_t cpuset_global;
pthread_mutex_t createLock;



/*******************************************************************************
 * API
 *******************************************************************************/
void tBenchServerInit(int nthreads) {
    
   	pthread_mutex_init(&createLock, nullptr);
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);
    int meta_thread_core = getOpt<int>("META_THREAD_CORE", 5);
    CPU_SET(meta_thread_core, &thread_cpu_set);
    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &thread_cpu_set) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    }

   // unsigned int coreID = sched_getcpu();
    // std::cout << "Confirm: Main thread running on " << coreID << '\n';

    #ifdef PER_REQ_MONITOR
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
    #endif

    std::cout << "----------Server Starting----------" << '\n';
    curTid = 0;
    std::string serverurl = getOpt<std::string>("TBENCH_SERVER", "");
    int serverport = getOpt<int>("TBENCH_SERVER_PORT", 7000);
    int nclients = getOpt<int>("TBENCH_NCLIENTS", 1);
    // std::cout << "TESTING: " << nthreads << " threads for server are detected\n";
    server = new NetworkedServer(nthreads, serverurl, serverport, nclients);
    std::cout << "----------Server Started----------" << '\n';
    #ifdef CONTROL_WITH_QLEARNING //set up the receiver thread for Q_learning
    tBenchSetup_thread();
    #endif
}

void tBenchServerThreadStart() {
    pthread_mutex_lock(&createLock);
    tid = curTid++;
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);
    std::string parsing_text;
    parsing_text = "SERVER_THREAD_" + std::to_string(tid) + "_CORE";
    int server_thread_core = getOpt<int>(parsing_text.c_str(), 6);
    CPU_SET(server_thread_core, &thread_cpu_set);
    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &thread_cpu_set) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    }
    pthread_mutex_unlock(&createLock);
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

#ifdef CONTROL_WITH_QLEARNING //implementtion of necessary functions for receiver thread

void* receive_thread_func(void *ptr)
{   
    NetworkedServer *server = (NetworkedServer*)ptr;
    int ret_val = 1;

    while(ret_val) 
    {
        ret_val = server->recvReq_Q();
    }

    return 0;
}

void tBenchSetup_thread()
{
    receive_thread = new pthread_t;
    attr = new pthread_attr_t;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    int controller_thread_core = getOpt<int>("CONTROLLER_THREAD_CORE", 7);
    CPU_SET(controller_thread_core,&cpuset);
    pthread_attr_init(attr);
    pthread_attr_setaffinity_np(attr,sizeof(cpu_set_t),&cpuset);
    pthread_create(receive_thread, attr, receive_thread_func, (void *)server);
  
}


void tBench_deleteReq() 
{
    delete global_req;
}

void tBench_join() 
{   
    pthread_join(*receive_thread,NULL);
}

#else
void tBench_join(){
    //do nothing
}
#endif
