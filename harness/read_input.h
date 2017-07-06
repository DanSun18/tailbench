#ifndef __READ_INPUT
#define __READ_INPUT

#include <fstream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>
#include <queue>
#include <iostream>
#include <string>
#include <unistd.h>

class QPScombo{
	private:
		uint64_t duration;
		double QPS;
	public:
		QPScombo(uint64_t dur, double _QPS){
			duration = dur;
			QPS = _QPS;
		}
		double getQPS(){return QPS;}
		uint64_t getDuration(){return duration;}
};

void readInput(std::string fileName);

std::queue<QPScombo*> QPStiming;

#endif