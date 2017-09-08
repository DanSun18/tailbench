#!/bin/bash
# $1 is the core number
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

NSERVERS=1
QPS=$3
WARMUPREQS=1000

TBENCH_QPS=${QPS} TBENCH_MINSLEEPNS=100000 TBENCH_SERVER=$2 \
  TBENCH_TERMS_FILE=${DATA_ROOT}/xapian/terms.in TBENCH_CLIENT_THREADS=2 taskset -c $1 /home/yl408/tailbench-v0.9/xapian/xapian_networked_client &
echo $! > client.pid

wait $(cat client.pid)

rm client.pid


