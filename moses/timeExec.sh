#!/bin/bash

#This script times how long spark runs given 
#the application and the cores; output is file [APPNAME].time
#it is best to taskset this application on a core
#not in conflict spark

APPNAME=$1
CORES=$2

if [ -z "$1" ]
	then
	echo "Please specify the application you want to run"
	exit 1
fi

if [ -z "$2" ]
	then
	echo "Please specify the cores kmeans will run on"
	exit 1
fi

echo "--------Starting ${APPNAME}-----------"
STARTTIME=$( date "+%s" )
source ~/spark_scripts/run_${APPNAME}.sh ${CORES}
wait $(cat ${APPNAME}.pid)
ENDTIME=$( date "+%s" )
EXECTIME=$(( ${ENDTIME} - ${STARTTIME} ))
echo -e ${STARTTIME} ${ENDTIME} ${EXECTIME} | tee -a ${APPNAME}.time  
echo "----------Running finished---------"
