#include "dynamic_qps_lookup.h"
#include "helpers.h"

#include <fstream>
#include <time.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <unistd.h>

DQPSLookup::DQPSLookup(std::string inputFile){
	started = false;
	std::ifstream infile(inputFile.c_str());
	//take input
	uint64_t duration;
	double QPS;
	while(infile >> duration >> QPS)
	{
		QPStiming.push(new QPScombo(duration, QPS));
	}
	startingNs = getCurNs();
}

double DQPSLookup::currentQPS(){
	
	if(QPStiming.empty())
		return -1;

	uint64_t currentNs = getCurNs();
	if(currentNs - startingNs >= (QPStiming.front()->getDuration())*1000*1000*1000){
		QPStiming.pop();
		if(QPStiming.empty())
			return -1;
		startingNs = getCurNs();
		return QPStiming.front()->getQPS();
	}
	else{
		return QPStiming.front()->getQPS();
	}
}

void DQPSLookup::setStartingNs(){
	if(!started)
	{
	started = true;
	startingNs = getCurNs();
	}
}