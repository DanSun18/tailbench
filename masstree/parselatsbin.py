#!/usr/bin/python

import numpy as np
import matplotlib.pyplot as plt
import sys
import csv 
input_file = sys.argv[1]
file = open(input_file)
service_time_list = list()
latency_time_list = list()

with file:
	lines = file.readlines()
	for line in lines:
		times = line.split(' ')
		service_time = float(times[0])
		latency_time = float(times[1])
		service_time_list.append(service_time/1000000)
		latency_time_list.append(latency_time/1000000)
##		print service_time/1000000,' ',latency_time/1000000 
	
sorted_service_time = np.sort(service_time_list)
sorted_latency_time = np.sort(latency_time_list)

sorted_service_time_list = list(sorted_service_time)
sorted_latency_time_list = list(sorted_latency_time)

#print list(sorted_service_time)
yvals_service = np.arange(len(sorted_service_time))/float(len(sorted_service_time))
#plt.plot(sorted_service_time,yvals_service)
#plt.show()

yvals_service_list = list(yvals_service)
file.close()

cdf_file = open(input_file+"_cdf",'w')

#for i in range(0,len(yvals_service_list)):
#	print str(sorted_service_time_list)+' '+str(yvals_service_list)+'\n'
#	cdf_file.write(str(sorted_service_time_list[i])+' '+str(yvals_service_list[i])+'\n') 


yvals_latency = np.arange(len(sorted_latency_time))/float(len(sorted_latency_time))
#plt.plot(sorted_latency_time,yvals_latency)
#plt.show()

yvals_latency_list = list(yvals_latency)

for i in range(0,len(yvals_latency_list)):
#	print str(sorted_service_time_list)+' '+str(yvals_service_list)+'\n'
	cdf_file.write(str(sorted_latency_time_list[i])+' '+str(yvals_latency_list[i])+'\n') 

	
