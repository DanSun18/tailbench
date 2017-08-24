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
    pthread_mutex_init(&pcmLock, nullptr);
    reqbuf = new Request[nthreads]; 

    activeFds.resize(nthreads);
    cstates.resize(nthreads);
    sktstates.resize(nthreads);
    recvClientHead = 0;

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


//arguments: id is thread id
size_t NetworkedServer::recvReq(int id, void** data, int core) {
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
	
	//std::cerr<<"reach here 3" <<std::endl;

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
        // When start to process each request, note down counter states with performance counter
        //find out core id current thread is on
        
        // unsigned int coreID = sched_getcpu();
        unsigned int socketID = 0; //it would be better for the thread to figure this out too
                            //but now using constant assuming it's always going to be 1
        //for debugging why instr and L3Miss sometimes ~= 2^64
        // std::cout << std::string("Thread ") << id << std::string(": received request ") << req->id << '\n';
        // std::cout << "\toperating on core " << coreID << '\n';

        pthread_mutex_lock(&pcmLock);
        CoreCounterState core_state = pcm->getCoreCounterState(core);
        SocketCounterState socket_state = pcm->getSocketCounterState(socketID);
        pthread_mutex_unlock(&pcmLock);
        cstates[id] = core_state;
        sktstates[id] = socket_state;
        *data = reinterpret_cast<void*>(&req->data);


        uint64_t curNs = getCurNs();
        reqInfo[id].id = req->id;
        reqInfo[id].startNs = curNs;
        activeFds[id] = fd;
        
    }

    pthread_mutex_unlock(&recvLock);

    return req->len;
};

void NetworkedServer::sendResp(int id, const void* data, size_t len, int core) {
    pthread_mutex_lock(&sendLock);

    Response* resp = new Response();
    
    resp->type = RESPONSE;
    resp->id = reqInfo[id].id;
    resp->len = len;
    memcpy(reinterpret_cast<void*>(&resp->data), data, len);

    uint64_t curNs = getCurNs();
    assert(curNs > reqInfo[id].startNs);
    resp->svcNs = curNs - reqInfo[id].startNs;
    resp->startNs = reqInfo[id].startNs;

    // finishing up request, find counter parameters
      //find out core id current thread is on
    // unsigned int coreID = sched_getcpu();
    unsigned int socketID = 0; //it would be better for the thread to figure this out too
                           //but now using constant assuming it's always going to be 1

    //for debugging why instr and L3Miss sometimes ~= 2^64
    // std::cout << "Thread " << id << ": sending response " << resp->id << '\n';
    // std::cout << "\toperating on core " <<  coreID << '\n';


    pthread_mutex_lock(&pcmLock);
    CoreCounterState core_state = pcm->getCoreCounterState(core);
    SocketCounterState socket_state = pcm->getSocketCounterState(socketID);
    pthread_mutex_unlock(&pcmLock);

    unsigned long int instrBefore = getInstructionsRetired(cstates[id]);
    // std::cout << "\tNumber of instructions on core counter before processing:" <<  instrBefore << '\n';
    unsigned long int instrAfter = getInstructionsRetired(core_state);
    // std::cout << "\tNumber of instructions on core counter after processing:" <<   instrAfter << '\n';

    //TODO: might need to look at L3 miss data too, but since they always occur together, for now just let them be


    unsigned long int instr = getInstructionsRetired(cstates[id], core_state);
    unsigned long int bytesRead = getBytesReadFromMC(sktstates[id], socket_state);
    unsigned long int bytesWritten = getBytesWrittenToMC(sktstates[id], socket_state);
    unsigned long int L3Miss = getL3CacheMisses(cstates[id], core_state);
    double L3HitRatio = getL3CacheHitRatio(cstates[id], core_state);

    

    resp->instr = instr;
    resp->bytesRead = bytesRead;
    resp->bytesWritten = bytesWritten;
    resp->L3MissNum = L3Miss;
    resp->L3HitRate = L3HitRatio;
    


    int fd = activeFds[id];
    int totalLen = sizeof(Response) - MAX_RESP_BYTES + len;
    int sent = sendfull(fd, reinterpret_cast<const char*>(resp), totalLen, 0);
    assert(sent == totalLen);

    ++finishedReqs;

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

/*******************************************************************************
 * Per-thread State
 *******************************************************************************/
__thread int tid;
__thread int coreId;

/*******************************************************************************
 * Global data
 *******************************************************************************/
std::atomic_int curTid;
NetworkedServer* server;
cpu_set_t cpuset_global;
pthread_mutex_t createLock;



/*******************************************************************************
 * API
 *******************************************************************************/
void tBenchServerInit(int nthreads) {
    //get cpu affinity of process
    // std::cout << "Initiating locck for creating threads" << '\n';
	pthread_mutex_init(&createLock, nullptr);
	// std:: cout << "ZEROing cpuset" << '\n';
    CPU_ZERO(&cpuset_global);

    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset_global) != 0){
        std::cerr << "sched_getaffinity failed" << '\n';
        exit(1);
    }
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);

    for (int c = 0; c < CPU_SETSIZE; ++c)
    {
        if (CPU_ISSET(c, &cpuset_global))
        {
            CPU_SET(c, &thread_cpu_set);
            CPU_CLR(c, &cpuset_global);
            // std::cout << "Pinning main thread to core " << c << '\n';
		break;
        }
    }

    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &thread_cpu_set) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    }

    unsigned int coreID = sched_getcpu();
    // std::cout << "Confirm: Main thread running on " << coreID << '\n';
    


    



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

    std::cout << "----------Server Starting----------" << '\n';
    curTid = 0;
    std::string serverurl = getOpt<std::string>("TBENCH_SERVER", "");
    int serverport = getOpt<int>("TBENCH_SERVER_PORT", 7000);
    int nclients = getOpt<int>("TBENCH_NCLIENTS", 1);
    // std::cout << "TESTING: " << nthreads << " threads for server are detected\n";
    server = new NetworkedServer(nthreads, serverurl, serverport, nclients);
    std::cout << "----------Server Started----------" << '\n';
}

void tBenchServerThreadStart() {
    pthread_mutex_lock(&createLock);
    tid = curTid++;
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);

    for (int c = 0; c < CPU_SETSIZE; ++c)
    {
        if (CPU_ISSET(c, &cpuset_global))
        {
            CPU_SET(c, &thread_cpu_set);
            CPU_CLR(c, &cpuset_global);
            coreId = c;
            // std::cout << "Pinning server thread " << tid << " to core " << c << '\n';
        	break;
	}
    }

    pthread_t thread;
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &thread_cpu_set) != 0)
    {
        std::cerr << "pthread_setaffinity_np failed" << '\n';
        exit(1);
    }
    // unsigned int coreID = sched_getcpu();
    // std::cout << "Confirm: server thread " << tid << " running on " << coreID << '\n';

    pthread_mutex_unlock(&createLock);
}

void tBenchServerFinish() {
	pcm->cleanup();
    server->finish();
}

size_t tBenchRecvReq(void** data) {
    return server->recvReq(tid, data, coreId);
}

void tBenchSendResp(const void* data, size_t size) {
    return server->sendResp(tid, data, size, coreId);
}

