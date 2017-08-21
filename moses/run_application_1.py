#!/usr/bin/python

import subprocess
from time import gmtime, strftime
from optparse import OptionParser
import os
import signal
from os.path import expanduser
import time

parser = OptionParser()
parser.add_option("-c","--core",help="cores for the LC application")
parser.add_option("-b","--batch",help="batch application co-running with the LC application,including:cc,correlation,decision_tree,fpgrowth,gradient,kmeans,linear_regression,movielens,naive_bayesian,pagerank,svm,triangle")
parser.add_option("-s","--batchcore",help="cores for the batch application")
parser.add_option("-d","--delay", help = "delay between batch and LC")
parser.add_option("-t","--tbenchqps",help="tbenchqps for moses")
parser.add_option("-f","--folder",help="specify location to store data, within data_sprak")
parser.add_option("-r","--ratio",help="ratio of MAXREQUEST over TBENCHQPS")
(options,args) = parser.parse_args()

current_time = strftime("%Y-%m-%d_%H:%M:%S",gmtime())
home = expanduser("~")
tbenchqps = 100
dataPath = "data_spark/"
ratio = 30
#print home+"/spark_scripts/run_"+options.batch+".sh"
if options.batch != None:
#	subprocess.call(["sudo", home+"/scripts/bash_scripts/set_freq.sh"])
	pro = subprocess.Popen([home+"/spark_scripts/run_"+options.batch+".sh",options.batchcore],preexec_fn=os.setsid)
	time.sleep(int(options.delay))
#subprocess.Popen(["sudo", home+"/scripts/bash_scripts/IntelPerformanceCounterMonitorV2.8/pcm.x","-r","-csv=result.csv","-i=36"])
#time.sleep(int(options.delay))
if options.tbenchqps != None:
	tbenchqps = (int)(options.tbenchqps)
if options.folder != None:
	dataPath = dataPath+options.folder+'/'
if options.ratio != None:
	ratio = (int)(options.ratio)
subprocess.call(["./run_networked.sh", str(2), str(ratio*tbenchqps), "0,1", str(tbenchqps), options.core, str(2*tbenchqps)])
if options.batch == None:
	subprocess.call(["cp","lats.bin","lats_"+options.core])
else:
	subprocess.call(["cp","lats.bin",dataPath+"lats_"+options.core+"_"+options.batch+"_"+options.batchcore+"_"+str(tbenchqps)+"_"+options.delay])

if options.batch != None:
	f = open(options.batch+".pid")
	pid = int(f.readline())
#	print pid
#	print pro.pid
	pro.kill()
#	print("before killpg")
	os.killpg(os.getpgid(pid),signal.SIGTERM)
#	print("after killpg")
	f.close()
#	print("after f.close")
if options.batch != None:
	subprocess.call(["rm",options.batch+".pid"])
#	print("after call rm")
#subprocess.call(["sudo", home+"/scripts/bash_scripts/reset_freq.sh"])
print("--------------------------------------running finished------------------------------------------------")
