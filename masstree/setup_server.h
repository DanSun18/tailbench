#!/bin/bash

if [[ -z "${NTHREADS}" ]]; then NTHREADS=2; fi

TBENCH_MAXREQS=36000 TBENCH_WARMUPREQS=1000 taskset -c $1  ./mttest_server_networked \
    -j${NTHREADS} mycsba masstree &
echo $! > server.pid
