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
#include <cstdlib>

#include "Util.h"

#define SCORE_STATS_MIN (numeric_limits<ScoreStatsType>::min())
#define ATOSST(str) ((ScoreStatsType) atoi(str))

#define scorebytes_ (entries_*sizeof(ScoreStatsType))

class ScoreStats
{
private:
	scorestats_t array_;		
	size_t entries_;
	size_t available_;
	
public:
	ScoreStats();
  ScoreStats(const size_t size);
	ScoreStats(const ScoreStats &stats);
	ScoreStats(std::string &theString);
	ScoreStats& operator=(const ScoreStats &stats);
	
	~ScoreStats();
	
	bool isfull(){return (entries_ < available_)?0:1; }
	void expand();
	void add(ScoreStatsType v);
		
	inline void clear() { memset((void*) array_,0,scorebytes_); }
	
	inline ScoreStatsType get(size_t i){ return array_[i]; }
	inline ScoreStatsType get(size_t i)const{ return array_[i]; }
	inline scorestats_t getArray() const { return array_; }
	
	void set(std::string &theString);

	inline size_t bytes() const{ return scorebytes_; }
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
	friend ostream& operator<<(ostream& o, const ScoreStats& e);
};


#endif

