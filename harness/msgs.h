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

#ifndef __MSGS_H
#define __MSGS_H

#include <stdint.h>
#include <stdlib.h>

// #define PER_REQ_MONITOR //uncomment to enable per request monitoring
// #define CONTROL_WITH_QLEARNING //uncomment to run with Q-Learning Controller

const int MAX_REQ_BYTES = 1 << 20; // 1 MB
const int MAX_RESP_BYTES = 1 << 20; // 1 MB

enum ResponseType { RESPONSE, ROI_BEGIN, FINISH };

struct Request {
    uint64_t id;
    uint64_t genNs;
    size_t len;
    char data[MAX_REQ_BYTES];
};

struct Response {
    ResponseType type;
    uint64_t id;
    uint64_t svcNs;
    uint64_t startNs;
    #ifdef PER_REQ_MONITOR
    unsigned int coreId;
    unsigned long int instr;
    unsigned long int bytesRead;
    unsigned long int bytesWritten;
    unsigned long int L3MissNum;
    double L3HitRate;
    uint64_t serverNs;
    uint64_t arrvNs;
    #endif
    size_t len;
    #ifdef CONTROL_WITH_QLEARNING //data to collect for Q Learning control
    size_t queue_len;
    #endif
    char data[MAX_RESP_BYTES];
};

#endif
