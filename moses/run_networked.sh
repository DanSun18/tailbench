#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=1
BINDIR=./bin

# Setup
cp moses.ini.template moses.ini
sed -i -e "s#@DATA_ROOT#$DATA_ROOT#g" moses.ini

# Launch Server
TBENCH_MAXREQS=15000 TBENCH_WARMUPREQS=1000 taskset -c 0,1 ${BINDIR}/moses_server_networked \
    -config ./moses.ini \
    -input-file ${DATA_ROOT}/moses/testTerms \
    -threads ${THREADS} -num-tasks 100000 -verbose 0 &

echo $! > server.pid
#$sudo chrt -f -p 99 $(cat server.pid)
sleep 5

# Launch Client

TBENCH_QPS=300 TBENCH_MINSLEEPNS=10000 TBENCH_CLIENT_THREADS=1 taskset -c 2,3  ${BINDIR}/moses_client_networked &
echo $! > client.pid

#sudo chrt -f -p 99 $(cat client.pid)

wait $(cat client.pid)

# Cleanup
./kill_networked.sh
rm server.pid client.pid
