/*
 *  ScoreArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_ARRAY_H
#define SCORE_ARRAY_H

#define SCORES_BEGIN "SCORES_BEGIN_0"
#define SCORES_END "SCORES_END_0"

using namespace std;

#include <limits>
#include <vector>
#include <iostream>
#include <fstream>

#include "Util.h"
#include "ScoreStats.h"

class ScoreArray
{
protected:
	vector<ScoreStats> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	int idx; // idx to identify the utterance, it can differ from the index inside the vector
	
public:
	ScoreArray();
	
	~ScoreArray(){};
		
	inline void clear() { array_.clear(); }

	inline size_t getIndex(){ return idx; }
	inline void setIndex(size_t value){ idx=value; }

	inline ScoreStats get(int i){ return array_.at(i); }
	void add(ScoreStats e){ array_.push_back(e); }
	
	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }
	
	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }

	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

};


#endif