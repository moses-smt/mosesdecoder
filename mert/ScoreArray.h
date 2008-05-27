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
	scorearray_t array_;
	
private:
	std::string  idx; // idx to identify the utterance, it can differ from the index inside the vector
	std::string score_type;
	
public:
	ScoreArray();
	
	~ScoreArray(){};
		
	inline void clear() { array_.clear(); }
	
	inline std::string getIndex(){ return idx; }
	inline void setIndex(const std::string& value){ idx=value; }

//	inline ScoreStats get(size_t i){ return array_.at(i); }
	
	inline ScoreStats&  get(size_t i){ return array_.at(i); }
	inline const ScoreStats&  get(size_t i)const{ return array_.at(i); }

	void add(const ScoreStats& e){ array_.push_back(e); }

	void merge(ScoreArray& e);

	inline std::string name(){ return score_type; };
	inline std::string name(std::string &sctype){ return score_type = sctype; };

	inline size_t size(){ return array_.size(); }
	inline size_t NumberOfScores(){ return (array_.size()>0)?array_.at(0).size():0; }
	
	void savetxt(ofstream& outFile, const std::string& sctype);
	void savebin(ofstream& outFile, const std::string& sctype);
	void save(ofstream& outFile, const std::string& sctype, bool bin=false);
	void save(const std::string &file, const std::string& sctype, bool bin=false);
	inline void save(const std::string& sctype, bool bin=false){ save("/dev/stdout", sctype, bin); }

	void loadtxt(ifstream& inFile);
	void loadbin(ifstream& inFile);
	void load(ifstream& inFile, bool bin=false);
	void load(const std::string &file, bool bin=false);
	
	bool check_consistency();
};


#endif

