#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

BINDIR=./bin


TBENCH_QPS=300 TBENCH_MINSLEEPNS=10000 TBENCH_CLIENT_THREADS=1 taskset -c 9-11  ${BINDIR}/moses_client_networked
