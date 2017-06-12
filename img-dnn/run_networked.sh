#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=1
REQS=100000000 # Set this very high; the harness controls maxreqs

TBENCH_WARMUPREQS=100 TBENCH_MAXREQS=100 ./img-dnn_server_networked \
    -r ${THREADS} -f ${DATA_ROOT}/models/model.xml -n 10000 &
echo $! > server.pid

sleep 2 # Wait for server to come up

TBENCH_QPS=500 TBENCH_MNIST_DIR=${DATA_ROOT}/mnist ./img-dnn_client_networked &

echo $! > client.pid
