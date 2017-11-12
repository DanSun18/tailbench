#!/bin/bash
# $1 is the core number
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh
if [ "$#" -ne 3 ]
then 
	echo "Usage:"
	echo "${BASH_SOURCE[0]} CORES TBENCH_SERVER QPS"
	exit 1
fi

NSERVERS=1
CORES=$1
TBENCH_SERVER=$2
QPS=$3
WARMUPREQS=1000

TBENCH_QPS=${QPS} TBENCH_MINSLEEPNS=100000 TBENCH_SERVER=${TBENCH_SERVER} \
  TBENCH_TERMS_FILE=${DATA_ROOT}/xapian/terms.in TBENCH_CLIENT_THREADS=2 taskset -c ${CORES} ${DIR}/xapian_networked_client &
echo $! > client.pid

wait $(cat client.pid)

rm client.pid


