#!/usr/bin/python

import subprocess
from time import gmtime, strftime
from optparse import OptionParser
import os
import signal
from os.path import expanduser
import time

parser = OptionParser()
parser.add_option("-l","--lc",help="LC application, including:xapian,masstree")
parser.add_option("-c","--core",help="cores for the LC application")
parser.add_option("-b","--batch",help="batch application co-running with the LC application,including:cc,correlation,decision_tree,fpgrowth,gradient,kmeans,linear_regression,movielens,naive_bayesian,pagerank,svm,triangle")
parser.add_option("-s","--batchcore",help="cores for the batch application")
parser.add_option("-d","--delay", help = "delay between batch and LC")
(options,args) = parser.parse_args()

current_time = strftime("%Y-%m-%d_%H:%M:%S",gmtime())
home = expanduser("~")
lc_path = home+"/tailbench-v0.9/"+options.lc
#print home+"/spark_scripts/run_"+options.batch+".sh"

#Start the spark process if number of pinned processors is nonzero
if options.batchcore != None:
	print options.batchcore
	pro = subprocess.Popen([home+"/spark_scripts/run_"+options.batch+".sh",options.batchcore], preexec_fn=os.setsid)
	time.sleep(int(options.delay))

#Start the lc process if number of pinned processors is nonzero
if options.core != '':
	subprocess.call([lc_path+"/run_networked.sh", options.core])
	subprocess.call(["mv","lats.bin","./experiment_results/lats_delay_"+options.delay+"_"+options.lc+"_"+options.core+"_"+options.batch+"_"+options.batchcore])

if options.batchcore != None:
	with open(options.batch+".pid",'r') as f:
		pid_string = f.readline()
		pid = int(pid_string)
		print pid
		#print pro.pid
		#pro.kill()
		print "before sigterm"
		os.killpg(os.getpgid(pid),signal.SIGTERM)
		#os.killpg(os.getpgid(pro.pid),signal.SIGTERM)
		print "after sigterm"
		#subprocess.call(["kill","-9",str(pid)])
	subprocess.call(["rm",options.batch+".pid"])
print("--------------------------------------running finished------------------------------------------------")
