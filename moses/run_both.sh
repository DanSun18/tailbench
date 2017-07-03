#!/bin/bash

#This application runs spark first, delays for some seconds
#then runs moses. It is best to pin the application to a core
# that minimizes interference

SERVER_THREADS=2
TBENCH_MAXREQS=$1
TBENCH_WARMUPREQS=$2
SERVER_CORES=21-23
TBENCH_QPS=$3
TBENCH_CLIENT_THREADS=1
CLIENT_CORES=9-11

BATCH_CORES=$4
BATCH_APP=$5
DELAY=$6


DATAFOLDER=$7 #within data_spark
FILENAME=$8
if [ $# -ne 8 ]; then
	echo -e "Please call the program with the in the following format: \nrun_both.sh [MAXREQS WARMUPREQS  QPS BATCHCORES BATCHAPP DELAY DATAFOLDER FILENAME]"
	exit 1
fi


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "---------run_both.sh starting---------"
echo -e "SERVER_THREADS=${SERVER_THREADS}\nTBENCH_MAXREQS=${TBENCH_MAXREQS}\nTBENCH_WARMUPREQS=${TBENCH_WARMUPREQS}\nSERVER_CORES=${SERVER_CORES}\nTBENCH_QPS=${TBENCH_QPS}\nTBENCH_CLIENT_THREADS=${TBENCH_CLIENT_THREADS}\nCLIENT_CORES=${CLIENT_CORES}\nBATCH_CORES=${BATCH_CORES}\nBATCH_APP=${BATCH_APP}\nDELAY=${DELAY}\nDATAFOLDER=${DATAFOLDER}" | tee ${FILENAME}.setup 
sudo echo "parameters parsed" #to avoid delay in batch or LC app
#Starting spark
if [ -x "${HOME}/spark_scripts/run_${BATCH_APP}.sh"  ]
then
	source ~/spark_scripts/run_${BATCH_APP}.sh ${BATCH_CORES}
else
	echo "File ~/spark_scripts/run_${BATCH_APP}.sh Not Found"
	exit 1
fi

sleep ${DELAY}

#run moses
source ./run_networked.sh ${SERVER_THREADS} ${TBENCH_MAXREQS} ${TBENCH_WARMUPREQS} ${SERVER_CORES} ${TBENCH_QPS} ${TBENCH_CLIENT_THREADS} ${CLIENT_CORES}

#kill spark
pkill -TERM -P $(cat ${BATCH_APP}.pid)
rm ${BATCH_APP}.pid
#move results and description
if ! [ -d "data_spark/${DATAFOLDER}" ]
then
	mkdir data_spark/${DATAFOLDER}
fi
mv lats.bin data_spark/${DATAFOLDER}/${FILENAME}.result
mv ${FILENAME}.setup data_spark/${DATAFOLDER}/.
#print finishing statement
echo "----------run_both.sh finished----------"
