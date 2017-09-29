#!/bin/bash
# $1 is the core number
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${DIR}/../configs.sh

NSERVERS=1
QPS=$2
WARMUPREQS=500
REQUESTS=$3

echo $NSERVERS 

#/home/yl408/scripts/bash_scripts/turnoff_HT.sh
TBENCH_MAXREQS=${REQUESTS} TBENCH_WARMUPREQS=${WARMUPREQS} \
 taskset -c $1  /home/yl408/tailbench/xapian/xapian_networked_server -n ${NSERVERS} -d ${DATA_ROOT}/xapian/wiki \
    -r 1000000000 &
echo $! > server.pid
cat server.pid
sudo chrt -f -p 99 $(cat server.pid)
# ssh yl408@clipper02 'cd ~/tailbench-v0.9/xapian/;TBENCH_QPS=600 TBENCH_MINSLEEPNS=100000 TBENCH_SERVER=10.148.54.60 TBENCH_TERMS_FILE=/home/yl408/tailbench.inputs/xapi#an/terms.in taskset -c 12-13 ./xapian_networked_client '

echo $QPS
TBENCH_QPS=${QPS} TBENCH_MINSLEEPNS=100000 TBENCH_SERVER= \

 TBENCH_TERMS_FILE=${DATA_ROOT}/xapian/terms.in TBENCH_CLIENT_THREADS=2 taskset -c 3-6  /home/yl408/tailbench/xapian/xapian_networked_client & 


echo $! > client.pid

sudo chrt -f -p 99 $(cat client.pid)

wait $(cat client.pid)

# Clean up
#./kill_networked.sh
#rm server.pid client.pid
#/home/yl408/scripts/bash_scripts/turnon_HT.sh                          
