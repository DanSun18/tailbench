#!/bin/bash

DATE=$2
THREAD=$1
DATAFILE=base_data/$DATE
LOGFILE=$DATAFILE/moses.log

echo -e "\n\n\n Service time:\n" | tee $LOGFILE	
for TBENCH_QPS in {100..1500..100}
do
	echo -e D$THREAD$TBENCH_QPS : | tee $LOGFILE
	./parselatsbin.py $DATAFILE/D$THREAD$TBENCH_QPS | tee $LOGFILE
	echo -e '\n' | tee $LOGFILE
done
	
