#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=1
REQS=100000000 # Set this very high; the harness controls maxreqs

TBENCH_WARMUPREQS=500 TBENCH_MAXREQS=2000 TBENCH_QPS=500 \
    TBENCH_MNIST_DIR=${DATA_ROOT}/img-dnn/mnist ./img-dnn_integrated -r ${THREADS} \
    -f ${DATA_ROOT}/img-dnn/models/model.xml -n ${REQS}
