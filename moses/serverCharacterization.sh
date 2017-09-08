#!/bin/bash

#This script is used to characterize the performance of server
#pin this to a core that is different from client and server
#sudo another command before executing to allow smooth operation

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DATADIR=${DIR}/data_base/Threads2

NTHREAD_SERVER=2
CORES_SERVER=21-23

NTHREAD_CLIENT=1
CORES_CLIENT=9-11

mkdir ${DATADIR}

sudo cpupower -c "${CORES_SERVER},${CORES_CLIENT}" frequency-set -g performance

for QPS in {100..1000..50}
do
	MAXREQS=$((30 * ${QPS}))
	WARMUPREQS=$((20 * ${QPS}))
	source ${DIR}/run_networked.sh ${NTHREAD_SERVER} ${MAXREQS} \
			${WARMUPREQS} ${CORES_SERVER} ${QPS} \
			${NTHREAD_CLIENT} ${CORES_CLIENT}
	sudo cp lats.bin ${DATADIR}/QPS${QPS}.bin
	echo -e "\n\n"
	sleep 5 #wait between execution
done

#restore cpu gorvernor
sudo cpupower -c "${CORES_SERVER},${CORES_CLIENT}" frequency-set -g ondemand

ANALYSISFILE=${DATADIR}/analysis.txt
touch ${ANALYSISFILE}

for QPS in {100..1000..50}
do
	echo -e QPS: ${QPS} | tee -a ${ANALYSISFILE}
	./parselatsbin.py ${DATADIR}/QPS${QPS}.bin | tee -a ${ANALYSISFILE}
	echo -e '' | tee -a ${ANALYSISFILE}
done
