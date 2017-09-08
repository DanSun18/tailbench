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
		service_time = float(times[1])
		latency_time = float(times[2])
		service_time_list.append(service_time/1000000)
		latency_time_list.append(latency_time/1000000)
##		print service_time/1000000,' ',latency_time/1000000 
	
sorted_service_time = np.sort(service_time_list)
sorted_latency_time = np.sort(latency_time_list)

sorted_service_time_list = list(sorted_service_time)
sorted_latency_time_list = list(sorted_latency_time)

#print list(sorted_service_time)
yvals_service = np.arange(len(sorted_service_time))/float(len(sorted_service_time))
yvals_latency = np.arange(len(sorted_latency_time))/float(len(sorted_latency_time))
#plt.plot(sorted_service_time,yvals_service)
#plt.show()

yvals_service_list = list(yvals_service)
yvals_latency_list = list(yvals_latency)
file.close()
service_cdf_file = open(input_file+"_cdf_service",'w')

#for i in range(0,len(yvals_service_list)):
#	print str(sorted_service_time_list)+' '+str(yvals_service_list)+'\n'
#	cdf_file.write(str(sorted_service_time_list[i])+' '+str(yvals_service_list[i])+'\n') 


#yvals_latency = np.arange(len(sorted_latency_time))/float(len(sorted_latency_time))
#plt.plot(sorted_latency_time,yvals_latency)
#plt.show()

#yvals_latency_list = list(yvals_latency)

index = 0 
for i in range(0,len(yvals_service_list)):
#	print str(sorted_service_time_list)+' '+str(yvals_service_list)+'\n'
	service_cdf_file.write(str(sorted_service_time_list[i])+' '+str(yvals_service_list[i])+'\n') 
	if (yvals_service_list[i] == 0.95):
		index = i
print "95 percentile service time: " + str(sorted_service_time_list[index])
service_cdf_file.close()

latency_cdf_file = open (input_file+"_cdf_latency",'w')
index = 0
for i in range(0,len(yvals_latency_list)):
	latency_cdf_file.write(str(sorted_latency_time_list[i])+' '+str(yvals_latency_list[i])+'\n')
	if(yvals_latency_list[i] == 0.95):
		index = i
print "95 percentile latency time: " + str(sorted_latency_time_list[index])
latency_cdf_file.close()
