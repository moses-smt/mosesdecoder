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

#define FEATURE_STATS_MIN (numeric_limits<FeatureStatsType>::min())
#define ATOFST(str) ((FeatureStatsType) atof(str))

#define featbytes_ (entries_*sizeof(FeatureStatsType))

class FeatureStats
{
private:
	featstats_t array_;		
	size_t entries_;
	size_t available_;
		
public:
	FeatureStats();
	FeatureStats(const size_t size);
	FeatureStats(const FeatureStats &stats);
	FeatureStats(std::string &theString);
	FeatureStats& operator=(const FeatureStats &stats);
	
	~FeatureStats();
	
	bool isfull(){return (entries_ < available_)?0:1; }
	void expand();
	void add(FeatureStatsType v);
		
	inline void clear() { memset((void*) array_,0,featbytes_); }
	
	inline FeatureStatsType get(size_t i){ return array_[i]; }
	inline FeatureStatsType get(size_t i)const{ return array_[i]; }
	inline featstats_t getArray() const { return array_; }

	void set(std::string &theString);

	inline size_t bytes() const{ return featbytes_; }
	inline size_t size() const{ return entries_; }
	inline size_t available() const{ return available_; }
	
	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	void savebin(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }
	
	void loadtxt(const std::string &file);
	void loadtxt(ifstream& inFile);
	void loadbin(ifstream& inFile);

	inline void reset(){ entries_ = 0; clear(); }
	
  /**write the whole object to a stream*/
  friend ostream& operator<<(ostream& o, const FeatureStats& e);
};

#endif


