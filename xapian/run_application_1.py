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
parser.add_option("-l","--load",help = "load to lc")
(options,args) = parser.parse_args()

current_time = strftime("%Y-%m-%d_%H:%M:%S",gmtime())
home = expanduser("~")
#print home+"/spark_scripts/run_"+options.batch+".sh"
if options.batch != None:
	subprocess.call(["sudo", home+"/scripts/bash_scripts/set_freq.sh"])
	pro = subprocess.Popen([home+"/spark_scripts/run_"+options.batch+".sh",options.batchcore],preexec_fn=os.setsid)
	time.sleep(int(options.delay))
#subprocess.Popen(["sudo", home+"/scripts/bash_scripts/IntelPerformanceCounterMonitorV2.8/pcm.x","-r","-csv=result.csv","-i=36"])
#time.sleep(int(options.delay))
subprocess.call(["./run_networked.sh", options.core,options.load])
if options.batch == None:
	subprocess.call(["mv","lats.bin","lats_"+options.core])
else:
	subprocess.call(["mv","lats.bin","lats_"+options.core+"_load_"+options.load+"_"+options.batch+"_"+options.batchcore+"_delay_"+options.delay])

if options.batch != None:
	f = open(options.batch+".pid")
	pid = int(f.readline())
#print pid
#print pro.pid
	pro.kill()
	os.killpg(os.getpgid(pid),signal.SIGTERM)
	f.close()
if options.batch != None:
	subprocess.call(["rm",options.batch+".pid"])
subprocess.call(["sudo", home+"/scripts/bash_scripts/reset_freq.sh"])
print("--------------------------------------running finished------------------------------------------------")
