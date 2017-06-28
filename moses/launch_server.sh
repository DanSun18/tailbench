#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=1
BINDIR=./bin

# Setup
cp moses.ini.template moses.ini
sed -i -e "s#@DATA_ROOT#$DATA_ROOT#g" moses.ini

# Launch Server
TBENCH_MAXREQS=6000 TBENCH_WARMUPREQS=4000 TBENCH_NCLIENTS=1 taskset -c 11 ${BINDIR}/moses_server_networked \
    -config ./moses.ini \
    -input-file /scratch/yl408/tailbench.inputs/moses/testTerms \
    -threads ${THREADS} -num-tasks 100000 -verbose 0
