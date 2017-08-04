#!/bin/bash

SERVER_THREADS=$1
TBENCH_MAXREQS=$2
TBENCH_WARMUPREQS=$3
SERVER_CORES=$4
TBENCH_QPS=$5
TBENCH_CLIENT_THREADS=$6
CLIENT_CORES=$7

if [ $# -ne 7 ]; then
        echo -e "Please call the program with the in the following format: \nrun_networked.sh [SERVERTHREADS MAXREQS WARMUPREQS SERVERCORES QPS CLIENTTHREADS CLIENTCORES]"
        exit 1
fi


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "DIR=${DIR}"
source ${DIR}/../configs.sh

BINDIR=${DIR}/bin

# Setup
cp ${DIR}/moses.ini.template ${DIR}/moses.ini
sed -i -e "s#@DATA_ROOT#$DATA_ROOT#g" ${DIR}/moses.ini

# Launch Server
TBENCH_MAXREQS=${TBENCH_MAXREQS} TBENCH_WARMUPREQS=${TBENCH_WARMUPREQS} TBENCH_NCLIENTS=1 \
taskset -c ${SERVER_CORES} ${BINDIR}/moses_server_networked \
    -config ${DIR}/moses.ini \
    -input-file ${DATA_ROOT}/moses/testTerms \
    -threads ${SERVER_THREADS} -num-tasks 7500000 -verbose 0 &

echo $! > ${DIR}/server.pid
sudo chrt -r -p 99 $(cat ${DIR}/server.pid)

sleep 5

# Launch Client
TBENCH_QPS=${TBENCH_QPS} TBENCH_MINSLEEPNS=10000 TBENCH_CLIENT_THREADS=${TBENCH_CLIENT_THREADS} taskset -c ${CLIENT_CORES}  ${BINDIR}/moses_client_networked &
echo $! > ${DIR}/client.pid

sudo chrt -f -p 99 $(cat ${DIR}/client.pid)

wait $(cat ${DIR}/server.pid)
wait $(cat ${DIR}/client.pid)

# Cleanup
rm ${DIR}/server.pid
rm ${DIR}/client.pid
