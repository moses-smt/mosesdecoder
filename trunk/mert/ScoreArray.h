/*
 *  ScoreArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_ARRAY_H
#define SCORE_ARRAY_H

#define SCORES_TXT_BEGIN "SCORES_TXT_BEGIN_0"
#define SCORES_TXT_END "SCORES_TXT_END_0"
#define SCORES_BIN_BEGIN "SCORES_BIN_BEGIN_0"
#define SCORES_BIN_END "SCORES_BIN_END_0"

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
	std::string score_type;
	
public:
	ScoreArray();
	
	~ScoreArray(){};
		
	inline void clear() { array_.clear(); }

	inline size_t getIndex(){ return idx; }
	inline void setIndex(size_t value){ idx=value; }

	inline ScoreStats get(int i){ return array_.at(i); }
	void add(ScoreStats e){ array_.push_back(e); }

	inline std::string name(){ return score_type; };
	inline std::string name(std::string &sctype){ return score_type = sctype; };

	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }
	
	void savetxt(ofstream& outFile);
	void savebin(ofstream& outFile);
	void save(ofstream& outFile, bool bin=false);
	void save(const std::string &file, bool bin=false);
	inline void save(bool bin=false){ save("/dev/stdout", bin); }

	void loadtxt(ifstream& inFile);
	void loadbin(ifstream& inFile);
	void load(ifstream& inFile, bool bin=false);
	void load(const std::string &file, bool bin=false);

};


#endif

