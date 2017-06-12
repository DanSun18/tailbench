#!/bin/bash
# ops-per-worker is set to a very large value, so that TBENCH_MAXREQS controls how
# many ops are performed
NUM_WAREHOUSES=1
NUM_THREADS=1

TBENCH_MAXREQS=400 TBENCH_WARMUPREQS=100 \
    ./out-perf.masstree/benchmarks/dbtest_server_networked --verbose --bench \
    tpcc --num-threads ${NUM_THREADS} --scale-factor ${NUM_WAREHOUSES} \
    --retry-aborted-transactions --ops-per-worker 100000 &

echo $! > server.pid

sleep 2 # Allow server to come up

TBENCH_QPS=5000 TBENCH_MINSLEEPNS=10000 \
    ./out-perf.masstree/benchmarks/dbtest_client_networked &

echo $! > client.pid

wait $(cat client.pid)

./kill_networked.sh
rm server.pid client.pid
