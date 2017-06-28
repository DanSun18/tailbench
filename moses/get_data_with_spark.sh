#!/bin/bash

FOLDER=$1
RATIO=$2
DATAPATH=data_spark/$FOLDER
LOGFILE=$DATAPATH/spark_moses.log
ANALYSISFILE=$DATAPATH/spark_moses.analysis
BATCH=kmeans
BATCHCORES=$3
CLIENTCORE=12
DELAY=$4
mkdir $DATAPATH
echo -e "THREAD =" $THREAD "\nBATCHCORES =" $BATCHCORES "\nCLIENT_CORE =" $CLIENT_CORE | tee -a $LOGFILE

for TBENCH_QPS in {100..1000..100}
do
	echo -e "TBENCH_QPS =" $TBENCH_QPS | tee -a $LOGFILE
	./run_application_1.py -c $CLIENTCORE -b $BATCH -s $BATCHCORES -d $DELAY -t $TBENCH_QPS -f $FOLDER -r $RATIO | tee -a $LOGFILE 
	echo -e '\n' | tee -a $LOGFILE
done

echo -e "\n\n\n Analysis:" | tee -a  $ANALYSISFILE	
for TBENCH_QPS in {100..1000..100}
do
	datafile="lats_${CLIENTCORE}_${BATCH}_${BATCHCORES}_${TBENCH_QPS}"
	echo -e $datafile : | tee -a $ANALYSISFILE
	./parselatsbin.py $DATAPATH/$datafile | tee -a $ANALYSISFILE
	echo -e '\n' | tee -a $ANALYSISFILE
done
