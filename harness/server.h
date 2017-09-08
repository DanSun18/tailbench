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

typedef struct {
    unsigned int Qlength;
    double service_time;
} state_info_t;

class Server {
    protected:
        struct ReqInfo {
        uint64_t id;
        uint64_t startNs;
	    uint64_t Qlength;
	    uint64_t Reqlen;
        uint64_t RecNs; // time when the request is received at the server
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

        Request *reqbuf; // One for each server thread

        std::vector<int> clientFds;
        std::vector<int> activeFds; // Currently active client fds for 
                                    // each thread
        size_t recvClientHead; // The idx of the client at the 'head' of the 
                               // receive queue. We start with this idx and go
                               // down the list of eligible fds to receive from.
                               // This is incremented by 1 on each go. This
                               // avoids unfairly favoring some clients over
                               // others
        int shm_fd, runLength;
        int sem_fd;
        int state_fd;
        sem_t *sem;
        void *shm_base;
        void *state_info_base;
        int starttime;
        state_info_t state_info;

        const char *name = "server_info";
        const char *state_name = "state_info";
        //const char *sem_name = "sem";
        void printDebugStats() const;

        // Helper Functions
        void removeClient(int fd);
        std::queue<Request*> recvReq_Queue;
	    std::queue<int> fd_Queue;
	    std::queue<int> Qlen_Queue;
        std::queue<uint64_t> rectime_Queue;
        std::vector<uint64_t> latencies;
        std::vector<uint64_t> services; 
        bool checkRecv(int recvd, int expected, int fd);
    public:

        NetworkedServer(int nthreads, std::string ip, int port, int nclients);
        ~NetworkedServer();

        int recvReq_Q();
        size_t recvReq(int id, void** data);
        void sendResp(int id, const void* data, size_t size);
        void finish();

        //for shared memory

        void init_mem();
        void update_mem();
        void update_server_info(unsigned int Qlength, float service_time);

};

#endif
