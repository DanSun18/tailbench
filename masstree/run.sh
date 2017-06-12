#!/bin/bash

if [[ -z "${NTHREADS}" ]]; then NTHREADS=1; fi

TBENCH_QPS=1000 TBENCH_MAXREQS=3000 TBENCH_WARMUPREQS=14000 \
    TBENCH_MINSLEEPNS=100000 ./mttest_integrated -j${NTHREADS} mycsba masstree
