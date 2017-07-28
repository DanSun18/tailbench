#ifndef __SERVER_H
#define __SERVER_H

#include <atomic>
#include <pthread.h>
#include <xapian.h>
#include <vector>
#include <queue>
#include <utility>
#include <sched.h>

class Server {
    private:
        static unsigned long numReqsToProcess;
        static volatile std::atomic_ulong numReqsProcessed;
        static const unsigned int MSET_SIZE = 20480;
        static pthread_barrier_t barrier;
        static std::queue<std::pair<void *,size_t>> request_queue;
        Xapian::Database db;
        Xapian::Enquire enquire;
        Xapian::Stem stemmer;
        Xapian::SimpleStopper stopper;
        Xapian::QueryParser parser;
        pthread_mutex_t lock;
        Xapian::MSet mset;

        int id;
	    int request_count;
        void _run();
        void processRequest();
        void receiveRequest();

    public:
        Server(int id, std::string dbPath);
        ~Server();

        static void* run(void* v);
        static void init(unsigned long _numReqsToProcess, unsigned numServers);
        static void* receive(void *v);
        static void fini();
};

#endif
