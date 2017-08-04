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

#ifndef __CLIENT_H
#define __CLIENT_H

#include "msgs.h"
#include "msgs.h"
#include "dist.h"

#include <pthread.h>
#include <stdint.h>

#include <string>
#include <unordered_map>
#include <vector>
// #include "dynamic_qps_lookup.h"


#include <stdlib.h>
#include <inttypes.h>
//#include <stdint.h>
#include <queue>
//#include <string>

enum ClientStatus { INIT, WARMUP, ROI, FINISHED };




class QPScombo{
    private:
        uint64_t duration;
        double QPS;
    public:
        QPScombo(uint64_t dur, double _QPS){
            duration = dur;
            QPS = _QPS;
        }
        double getQPS(){return QPS;}
        uint64_t getDuration(){return duration;}
};

class DQPSLookup{
    private:
        std::queue<QPScombo*> QPStiming;
        uint64_t startingNs; //starting time of the current QPS period
        bool started;
    public:
        DQPSLookup(std::string inputFile);
        double currentQPS();
        void setStartingNs();
};

class Client {
    protected:
        ClientStatus status;

        int nthreads;
        pthread_mutex_t lock;
        pthread_barrier_t barrier;

        uint64_t minSleepNs;
        uint64_t seed;
        double lambda;
        double current_qps;
        ExpDist* dist;

        uint64_t startedReqs;
        std::unordered_map<uint64_t, Request*> inFlightReqs;

        std::vector<uint64_t> svcTimes;
        std::vector<uint64_t> queueTimes;
        std::vector<uint64_t> sjrnTimes;
	std::vector<uint64_t> startTimes;

        std::queue< std::vector<uint64_t> > _svcTimes;
        std::queue< std::vector<uint64_t> > _queueTimes;
        std::queue< std::vector<uint64_t> > _sjrnTimes;
	std::queue<std::vector<uint64_t> > _startTimes;
        std::queue<double> QPSSequence;

        void _startRoi();

        DQPSLookup dqpsLookup;



    public:
        Client(int nthreads);

        Request* startReq();
        void finiReq(Response* resp);

        void startRoi();
        void dumpStats();
        void dumpAllStats();

};

class NetworkedClient : public Client {
    private:
        pthread_mutex_t sendLock;
        pthread_mutex_t recvLock;

        int serverFd;
        std::string error;

    public:
        NetworkedClient(int nthreads, std::string serverip, int serverport);
        bool send(Request* req);
        bool recv(Response* resp);
        const std::string& errmsg() const { return error; }
};

#endif
