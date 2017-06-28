#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=$1 #for server
BINDIR=./bin

# Setup
cp moses.ini.template moses.ini
sed -i -e "s#@DATA_ROOT#$DATA_ROOT#g" moses.ini

# Launch Server
TBENCH_MAXREQS=$2 TBENCH_WARMUPREQS=$3 TBENCH_NCLIENTS=1 \
taskset -c $4 ${BINDIR}/moses_server_networked \
    -config ./moses.ini \
    -input-file ${DATA_ROOT}/moses/testTerms \
    -threads ${THREADS} -num-tasks 100000 -verbose 0 &

echo $! > server.pid
sudo chrt -r -p 99 $(cat server.pid)

sleep 5

# Launch Client
TBENCH_QPS=$5 TBENCH_MINSLEEPNS=10000 TBENCH_CLIENT_THREADS=$6 taskset -c $7  ${BINDIR}/moses_client_networked &
echo $! > client.pid

sudo chrt -f -p 99 $(cat client.pid)

wait $(cat client.pid)

# Cleanup
source ./kill_networked.sh
