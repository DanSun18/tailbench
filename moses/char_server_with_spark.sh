#!/bin/bash

#This application runs spark first, delays for some seconds
#then runs moses. It is best to pin the application to a core
# that minimizes interference

BATCH_CORES=$1
BATCH_APP=$2
DELAY=$3
DATAFOLDER=$4

if [ $# -ne 4 ]; then
        echo -e "Please call the program with the in the following format: \nchar_server_with_spark.sh [BATCHCORES BATCHAPP DELAY DATAFOLDER]"
        exit 1
fi
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir data_spark/${DATAFOLDER}

sudo cpupower -c 6-23 frequency-set -g performance

for QPS in {100..1000..50}
do
	MAXREQS=$((30 * ${QPS}))
        WARMUPREQS=$((20 * ${QPS}))	
	source ./run_both.sh ${MAXREQS} ${WARMUPREQS} ${QPS} ${BATCH_CORES} ${BATCH_APP} ${DELAY} ${DATAFOLDER} "QPS${QPS}" 
	echo -e '\n'
	sleep 5
done

sudo cpupower frequency-set -g ondemand 

ANALYSISFILE=data_spark/${DATAFOLDER}/analysis.txt
touch ${ANALYSISFILE}

for QPS in {100..1000..50}
do
        echo -e QPS: ${QPS} | tee -a ${ANALYSISFILE}
        ./parselatsbin.py data_spark/${DATAFOLDER}/QPS${QPS}.result | tee -a ${ANALYSISFILE}
        echo -e '' | tee -a ${ANALYSISFILE}
done

