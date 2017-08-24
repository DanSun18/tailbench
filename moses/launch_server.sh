#!/bin/bash

SERVER_THREADS=$1
TBENCH_MAXREQS=$2
TBENCH_WARMUPREQS=$3
SERVER_CORES=$4

if [ $# -ne 4 ]; then
        echo -e "Please call the program with the in the following format:"
        echo -e "${BASH_SOURCE[0]} [SERVERTHREADS MAXREQS WARMUPREQS SERVERCORES]"
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
TBENCH_SERVER=$(hostname) \
taskset -c ${SERVER_CORES} ${BINDIR}/moses_server_networked \
    -config ${DIR}/moses.ini \
    -input-file ${DATA_ROOT}/moses/testTerms \
    -threads ${SERVER_THREADS} -num-tasks 7500000 -verbose 0 &

echo $! > ${DIR}/server.pid

sudo chrt -r -p 99 $(cat ${DIR}/server.pid)

wait $(cat ${DIR}/server.pid)

# Cleanup
rm ${DIR}/server.pid
