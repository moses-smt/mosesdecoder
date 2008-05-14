/*
 *  FeatureStats.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_STATS_H
#define FEATURE_STATS_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"

typedef float FeatureStatsType;

#define FEATURE_STATS_MIN (numeric_limits<FeatureStatsType>::min())
#define ATOFST(str) ((FeatureStatsType) atof(str))

class FeatureStats
{
protected:
	vector<FeatureStatsType> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	
public:
	FeatureStats();
        FeatureStats(const size_t size);
	FeatureStats(const FeatureStats &stats);
	FeatureStats(std::string &theString);
	FeatureStats& operator=(const FeatureStats &stats);
	
	~FeatureStats(){};
		
	inline void clear() { array_.clear(); }
	
	inline FeatureStatsType get(int i){ return array_.at(i); }
	
	void set(std::string &theString);

	void add(FeatureStatsType e){ array_.push_back(e); }

	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }

	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }
	
	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

	inline void reset()
	{
		for (vector<FeatureStatsType>::iterator i = array_.begin(); i != array_.end(); i++)
			*i = 0;
	}
		
	/*
	 * Marshalls this classes data members into a single
	 * contiguous memory location for the purpose of storing
	 * the data in a database.
	 */
	char *getBuffer();	
	
	void setBuffer(char* buffer, size_t sz);
	
	int pack(char *buffer, size_t &bufferlen);

	int unpack(char *buffer, size_t &bufferlen);

};


#endif