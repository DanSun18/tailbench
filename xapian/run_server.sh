#!/bin/bash
# $1 is the core number
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh
if [ "$#" -ne 3 ]
then 
	echo "Usage:"
	echo "${BASH_SOURCE[0]} CORES QPS REQUESTS"
	exit 1
fi
NSERVERS=1
CORES=$1
QPS=$2
WARMUPREQS=500
REQUESTS=$3

echo $NSERVERS 

#/home/yl408/scripts/bash_scripts/turnoff_HT.sh
TBENCH_MAXREQS=${REQUESTS} TBENCH_WARMUPREQS=${WARMUPREQS} \
 taskset -c ${CORES}  /home/yl408/tailbench/xapian/xapian_networked_server -n ${NSERVERS} -d ${DATA_ROOT}/xapian/wiki \
    -r 1000000000 &
echo $! > server.pid
cat server.pid
sudo chrt -f -p 99 $(cat server.pid)
rm server.pid                      
