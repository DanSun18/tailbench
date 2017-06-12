#!/bin/bash

if [[ -z "${NTHREADS}" ]]; then NTHREADS=2; fi

TBENCH_MAXREQS=36000 TBENCH_WARMUPREQS=1000 taskset -c $1  ./mttest_server_networked \
    -j${NTHREADS} mycsba masstree &
echo $! > server.pid

sleep 5 # Allow server to come up

TBENCH_QPS=500 TBENCH_MINSLEEPNS=100000 ./mttest_client_networked &
echo $! > client.pid

wait $(cat client.pid)
    
