#!/usr/bin/python
import subprocess
from time import gmtime, strftime
from optparse import OptionParser
import os
import signal
from os.path import expanduser
import time
import numpy as np
import matplotlib.pyplot as plt
import sys
import csv 

parser = OptionParser()
parser.add_option("-n","--name",help="")

(options,args) = parser.parse_args()


prefix = ["0-1,24-25_","0-3,24-27_","0-5,24-29_","0-7,24-31_","0-9,24-33_","0-11,24-35_"]

#file_name_prefix = ["lats_"+ s + str(options.name) for s in prefix]
#file_name = list()
#sufix = ["_2-11,26-35_cdf","_4-11,28-35_cdf","_6-11,30-35_cdf","_8-11,32-35_cdf","_10-11,34-35_cdf","_12_cdf"]

#for i in range(0,len(file_name_prefix)):
#        file_name.append(file_name_prefix[i]+sufix[i])

#file_name = ['lats_0-5,24-29_linear_regression_6-11,30-35_1_cdf','lats_0-5,24-29_linear_regression_6-11,30-35_2_cdf','lats_0-5,24-29_linear_regression_6-11,30-35_3_cdf','lats_0-5,24-29_linear_regression_6-11,30-35_4_cdf','lats_0-5,24-29_linear_regression_6-11,30-35_5_cdf','lats_0-5,24-29_linear_regression_6-11,30-35_6_cdf']


file_name = ['lats_kv_24cores_1t_cdf','lats_kv_24cores_2t_cdf','lats_kv_24cores_4t_cdf']

#file_name = ['lats_bin_1_cdf']
files = [open(f) for f in file_name]

#ninetyfiveth = [0.95]*12000
yvals = list()
sorted_service_times = list()
#ninetyninth = [0.99]*12000
for file in files:
	lines = file.readlines()
	sorted_service_time = list()
	yval = list()
	for line in lines:
		line = line.replace('/n','')
		vals = line.split(' ')
		service_time = float(vals[0])
		val = float(vals[1])
		sorted_service_time.append(service_time)
		yval.append(val)

	sorted_service_times.append(sorted_service_time)
	yvals.append(yval)	



fig,ax=plt.subplots()
#print max(sorted_service_times[0])
#print max(sorted_service_times[1])
#ax.plot(sorted_service_times[0],yvals[0],label="LC:2 cores BE:10 cores")
#ax.plot(sorted_service_times[1],yvals[1],label="LC:4 cores BE:8 cores")
#ax.plot(sorted_service_times[2], yvals[2],label="LC:6 cores BE:6 cores")
#ax.plot(sorted_service_times[3],yvals[3],label = "LC:8 cores BE:4 cores")
#ax.plot(sorted_service_times[4],yvals[4],label = "LC:10 cores BE:2 cores")
#ax.plot(sorted_service_times[5],yvals[5],label = "LC:12 cores")
#ax.plot(sorted_service_times[4],yvals[4],label = "fifth time")

for i in range(0,len(files)):
	ax.plot(sorted_service_times[i],yvals[i],label=file_name[i])

#ax.plot(sorted_service_times[0],yvals[0],label="1st")
#ax.plot(sorted_service_times[1],yvals[1],label="2nd")
#ax.plot(sorted_service_times[2], yvals[2],label="3rd")
#ax.plot(sorted_service_times[3],yvals[3],label = "4th")
#ax.plot(sorted_service_times[3],yvals[3],label = "5th")
#ax.plot(sorted_service_times[3],yvals[3],label = "6th")
#ax.plot(sorted_service_times[3],ninetyfiveth,'k:')
#ax.plot(sorted_service_times[4],yvals[4],label = "8 cores")
#ax.plot(sorted_service_times[5],yvals[5], label="24 cores")
range_x = ax.get_xlim()
range_plot = range(int(range_x[0]),int(range_x[1])+1)

ninetyfiveth = [0.95]*len(range_plot)
ninetyninth = [0.99]*len(range_plot)
ax.plot(range_plot,ninetyfiveth,'k:')
ax.plot(range_plot,ninetyninth,'k:')


legend = ax.legend(loc='upper right', shadow=True)
# The frame is matplotlib.patches.Rectangle instance surrounding the legend.
frame = legend.get_frame()
frame.set_facecolor('0.90')

# Set the fontsize
for label in legend.get_texts():
    label.set_fontsize('large')

for label in legend.get_lines():
    label.set_linewidth(1.5)  # the legend line width

plt.ylim([0.6,1])
plt.yticks([0.6,0.8,0.95,0.99,1])
plt.ylabel("cumulative distribution")
plt.xlabel("service[ms]")
plt.show()
