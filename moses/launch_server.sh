#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

THREADS=2
BINDIR=./bin

# Setup
cp moses.ini.template moses.ini
sed -i -e "s#@DATA_ROOT#$DATA_ROOT#g" moses.ini

# Launch Server
TBENCH_MAXREQS=0 TBENCH_WARMUPREQS=2000 TBENCH_NCLIENTS=1 taskset -c 21-23 ${BINDIR}/moses_server_networked \
    -config ./moses.ini \
    -input-file /scratch/yl408/tailbench.inputs/moses/testTerms \
    -threads ${THREADS} -num-tasks 7500000 -verbose 0
