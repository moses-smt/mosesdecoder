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

class FeatureStats
{
protected:
	featstats_t array_;
	
private:
	
public:
	FeatureStats();
	FeatureStats(const size_t size);
	FeatureStats(const FeatureStats &stats);
	FeatureStats(std::string &theString);
	FeatureStats& operator=(const FeatureStats &stats);
	
	~FeatureStats(){};
		
	inline void clear() { array_.clear(); }
	
	inline FeatureStatsType get(size_t i){ return array_.at(i); }
	inline FeatureStatsType get(size_t i)const{ return array_.at(i); }
	inline featstats_t getArray() const { return array_; }
	
	void set(std::string &theString);

	void add(FeatureStatsType e){ array_.push_back(e); }

	inline size_t size(){ return array_.size(); }
	
	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	void savebin(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }
	
	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

	inline void reset()
	{
		for (featstats_t::iterator i = array_.begin(); i != array_.end(); i++)
			*i = 0;
	}
		
	
  /**write the whole object to a stream*/
  friend ostream& operator<<(ostream& o, const FeatureStats& e);

};

#endif


