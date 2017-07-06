#ifndef __DYNAMIC_QPS_LOOKUP
#define __DYNAMIC_QPS_LOOKUP

#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <queue>
#include <string>


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

class DQPSLookup{
	private:
		std::queue<QPScombo*> QPStiming;
		uint64_t startingNs; //starting time of the current QPS period
		bool started;
	public:
		DQPSLookup(std::string inputFile);
		double currentQPS();
		void setStartingNs();
};


#endif