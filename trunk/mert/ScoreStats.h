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

typedef int ScoreStatsType;

#define SCORE_STATS_MIN (numeric_limits<ScoreStatsType>::min())
#define ATOSST(str) ((ScoreStatsType) atoi(str))

class ScoreStats
{
protected:
	vector<ScoreStatsType> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	
public:
	ScoreStats();
        ScoreStats(const size_t size);
	ScoreStats(const ScoreStats &stats);
	ScoreStats(std::string &theString);
	ScoreStats& operator=(const ScoreStats &stats);
	
	~ScoreStats(){};
		
	inline void clear() { array_.clear(); }
	
	inline ScoreStatsType get(int i){ return array_.at(i); }
	
	void set(std::string &theString);

	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }

	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }
	
	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

	inline void reset()
	{
		for (vector<ScoreStatsType>::iterator i = array_.begin(); i != array_.end(); i++)
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

