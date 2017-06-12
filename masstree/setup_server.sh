#!/bin/bash

if [[ -z "${NTHREADS}" ]]; then NTHREADS=$2; fi

TBENCH_MAXREQS=120000 TBENCH_WARMUPREQS=3000 taskset -c $1  ./mttest_server_networked \
    -j${NTHREADS} mycsba masstree &
 
echo $! > server.pid

sudo chrt -f -p 99 $(cat server.pid)
