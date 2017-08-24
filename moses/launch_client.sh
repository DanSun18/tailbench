#!/bin/bash

TBENCH_QPS=$1
TBENCH_CLIENT_THREADS=$2
CLIENT_CORES=$3
TBENCH_SERVER=$4

if [ $# -ne 4 ]; then
        echo -e "Please call the program with the in the following format:"
        echo -e "${BASH_SOURCE[0]} [TBENCH_QPS TBENCH_CLIENT_THREADS CLIENT_CORES TBENCH_SERVER]"
        exit 1
fi


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "DIR=${DIR}"
source ${DIR}/../configs.sh

BINDIR=${DIR}/bin

# Launch Client
TBENCH_QPS=${TBENCH_QPS} TBENCH_MINSLEEPNS=10000 \
TBENCH_CLIENT_THREADS=${TBENCH_CLIENT_THREADS} TBENCH_SERVER=${TBENCH_SERVER} \
taskset -c ${CLIENT_CORES}  ${BINDIR}/moses_client_networked &
echo $! > ${DIR}/client.pid

sudo chrt -f -p 99 $(cat ${DIR}/client.pid)

wait $(cat ${DIR}/client.pid)

# Cleanup
rm ${DIR}/client.pid
