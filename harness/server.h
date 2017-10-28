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

#ifndef __SERVER_H
#define __SERVER_H

#include "client.h"
#include "dist.h"
#include "helpers.h"
#include "msgs.h"

#include <pthread.h>
#include <stdint.h>

#include <unordered_map>
#include <vector>

#ifdef CONTROL_WITH_QLEARNING //necessary files for q_learning

#include <queue>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <semaphore.h>
#include <deque>

typedef struct {
    unsigned int window_id;
    unsigned int Qlength;
    double service_time;
} state_info_t;
#endif 



#ifdef PER_REQ_MONITOR
#include <unistd.h>
#include <signal.h>   // for atexit()
#include <sys/time.h> // for gettimeofday()
#include <math.h>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <sstream>
#include <assert.h>
#include "../pcm-master/cpucounters.h"
#include "../pcm-master/utils.h"

PCM * pcm;
#endif

class Server {
    protected:
        struct ReqInfo {
            uint64_t id;
            uint64_t startNs;
            #ifdef CONTROL_WITH_QLEARNING //info for Q learning
	       uint64_t Qlength;
	        uint64_t Reqlen;
            uint64_t RecNs; // time when the request is received at the server
           #endif

            #ifdef PER_REQ_MONITOR
            uint64_t arrvNs;
            #endif
        };

        uint64_t finishedReqs;
        uint64_t maxReqs;
        uint64_t warmupReqs;

        std::vector<ReqInfo> reqInfo; // Request info for each thread 
        


    public:
        Server(int nthreads) {
            finishedReqs = 0;
            maxReqs = getOpt("TBENCH_MAXREQS", 0);
            warmupReqs = getOpt("TBENCH_WARMUPREQS", 0);
            reqInfo.resize(nthreads);
        }

        virtual size_t recvReq(int id, void** data) = 0;
        virtual void sendResp(int id, const void* data, size_t size) = 0;
};

class IntegratedServer : public Server, public Client {
    public:
        IntegratedServer(int nthreads);

        size_t recvReq(int id, void** data);
        void sendResp(int id, const void* data, size_t size);
};

class NetworkedServer : public Server {
    private:
        pthread_mutex_t sendLock;
        pthread_mutex_t recvLock;
        #ifdef PER_REQ_MONITOR
        pthread_mutex_t pcmLock;
        #endif
        Request *reqbuf; // One for each server thread

        std::vector<int> clientFds;
        std::vector<int> activeFds; // Currently active client fds for 
                                    // each thread
        //state1 stores counter state when start processing request, socket 2 store when finishing
        #ifdef PER_REQ_MONITOR
        std::vector<CoreCounterState> cstates;
        std::vector<SocketCounterState> sktstates;
        #endif
        size_t recvClientHead; // The idx of the client at the 'head' of the 
                               // receive queue. We start with this idx and go
                               // down the list of eligible fds to receive from.
                               // This is incremented by 1 on each go. This
                               // avoids unfairly favoring some clients over
                               // others
        void printDebugStats() const;

        // Helper Functions
        void removeClient(int fd);
        bool checkRecv(int recvd, int expected, int fd);

        #ifdef CONTROL_WITH_QLEARNING //variables and constants necessary for Q Learning
        int server_info_fd;
        int sem_fd;
        int state_info_fd;
        int runLength;
        sem_t *sem;
        void *server_info_mem_addr;
        void *state_info_mem_addr;
        int starttime; //the start time of each window
        unsigned int current_window_id; 
        state_info_t state_info;

        const char *server_info_shm_file_name = "server_info";
        const char *state_info_shm_file_name = "state_info";
        //const char *sem_name = "sem";
        
        std::queue<Request*> recvReq_Queue; // data structure for holding unprocessed requests
	    std::queue<int> fd_Queue; //keep record of fd to return for request
	    std::deque<unsigned int> Qlen_Queue; //keep record of queue length when request is received
        std::queue<uint64_t> rectime_Queue; // keep record of when the request is received
        //variables for every window
        std::vector<uint64_t> latencies; //latencies in current window
        std::vector<uint64_t> services;  //service times in current window
        #endif

    public:

        NetworkedServer(int nthreads, std::string ip, int port, int nclients);
        ~NetworkedServer();

        
        size_t recvReq(int id, void** data);
        void sendResp(int id, const void* data, size_t size);
        void finish();
        
        #ifdef CONTROL_WITH_QLEARNING //methods for Q Learning
        int recvReq_Q();
        //for shared memory

        void init_shm();
        void update_mem();
        void update_server_info(unsigned int Qlength, float service_time);
        #endif
};

#endif
