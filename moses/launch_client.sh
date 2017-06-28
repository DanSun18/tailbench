#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

BINDIR=./bin


TBENCH_QPS=200 TBENCH_MINSLEEPNS=10000 TBENCH_CLIENT_THREADS=2 taskset -c 12-23  ${BINDIR}/moses_client_networked
