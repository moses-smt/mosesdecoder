/*
 *  ScoreStats.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_STATS_H
#define SCORE_STATS_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"

#define SCORE_STATS_MIN (numeric_limits<ScoreStatsType>::min())
#define ATOSST(str) ((ScoreStatsType) atoi(str))

class ScoreStats
{
protected:
	scorestats_t array_;
	
private:
	
public:
	ScoreStats();
  ScoreStats(const size_t size);
	ScoreStats(const ScoreStats &stats);
	ScoreStats(std::string &theString);
	ScoreStats& operator=(const ScoreStats &stats);
	
	~ScoreStats(){};
		
	inline void clear() { array_.clear(); }
	
	inline ScoreStatsType get(size_t i){ return array_.at(i); }
	inline ScoreStatsType get(size_t i)const{ return array_.at(i); }
	inline scorestats_t getArray() const { return array_; }
	
	void set(std::string &theString);

	inline size_t size(){ return array_.size(); }

	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }
	
	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

	inline void reset()
	{
		for (scorestats_t::iterator i = array_.begin(); i != array_.end(); i++)
			*i = 0;
	}

	/**write the whole object to a stream*/
	friend ostream& operator<<(ostream& o, const ScoreStats& e);
};


#endif

