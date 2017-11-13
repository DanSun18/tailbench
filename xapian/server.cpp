#include <cstring>
#include <iostream>
#include <sstream>

#include <assert.h>
#include <unistd.h>

#include "server.h"
#include "tbench_server.h"

#include <pthread.h>

using namespace std;

unsigned long Server::numReqsToProcess = 0;
volatile atomic_ulong Server::numReqsProcessed(0);
pthread_barrier_t Server::barrier;

Server::Server(int id, string dbPath) 
    : db(dbPath)
    , enquire(db)
    , stemmer("english")
    , id(id)
{
    const char* stopWords[] = { "a", "about", "an", "and", "are", "as", "at", "be",
        "by", "en", "for", "from", "how", "i", "in", "is", "it", "of", "on",
        "or", "that", "the", "this", "to", "was", "what", "when", "where",
        "which", "who", "why", "will", "with" };

    stopper = Xapian::SimpleStopper(stopWords, \
            stopWords + sizeof(stopWords) / sizeof(stopWords[0]));

    parser.set_database(db);
    parser.set_default_op(Xapian::Query::OP_OR);
    parser.set_stemmer(stemmer);
    parser.set_stemming_strategy(Xapian::QueryParser::STEM_SOME);
    parser.set_stopper(&stopper);
}

Server::~Server() {
}

void Server::_run() {
    pthread_barrier_wait(&barrier);

    tBenchServerThreadStart();
    std::cerr << "Setting thread affinity inside xapian" << "\n";
    cpu_set_t thread_cpu_set;
    CPU_ZERO(&thread_cpu_set);
    int server_thread_core = 1;
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
    std::cerr << "finished setting thread affinity in xapian" << "\n";

    while (numReqsProcessed < numReqsToProcess) {
       processRequest();
       ++numReqsProcessed;
    }
}

void Server::processRequest() {
    const unsigned MAX_TERM_LEN = 256;
    char term[MAX_TERM_LEN];
    void* termPtr;

    size_t len = tBenchRecvReq(&termPtr);
    memcpy(reinterpret_cast<void*>(term), termPtr, len);
    term[len] = '\0';
    #ifdef CONTROL_WITH_QLEARNING
    tBench_deleteReq();
    #endif
    //std::cerr << "reach here ! " << std::endl;
    unsigned int flags = Xapian::QueryParser::FLAG_DEFAULT;
    Xapian::Query query = parser.parse_query(term, flags);
    enquire.set_query(query);
    mset = enquire.get_mset(0, MSET_SIZE);

    const unsigned MAX_RES_LEN = 1 << 20;
    char res[MAX_RES_LEN];

    unsigned resLen = 0;
    unsigned doccount = 0;
    const unsigned MAX_DOC_COUNT = 25; // up to 25 results per page
    for (auto it = mset.begin(); it != mset.end(); ++it) {
        std::string desc = it.get_document().get_description();
        resLen += desc.size();
        assert(resLen <= MAX_RES_LEN);
        memcpy(reinterpret_cast<void*>(&res[resLen]), desc.c_str(), desc.size());

        if (++doccount == MAX_DOC_COUNT) break;
    }
    //std::cerr << "reach here finish processing" <<std::endl;
    tBenchSendResp(reinterpret_cast<void*>(res), resLen);
}

void* Server::run(void* v) {
    Server* server = static_cast<Server*> (v);
    server->_run();
    return NULL;
}

void Server::init(unsigned long _numReqsToProcess, unsigned numServers) {
    numReqsToProcess = _numReqsToProcess;
    pthread_barrier_init(&barrier, NULL, numServers);
}

void Server::fini() {
    pthread_barrier_destroy(&barrier);
}
