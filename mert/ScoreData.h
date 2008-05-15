/*
 *  ScoreData.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_DATA_H
#define SCORE_DATA_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"
#include "ScoreArray.h"

class Scorer;

class ScoreData
{
protected:
	vector<ScoreArray> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	Scorer* theScorer;	
	std::string score_type;
	
public:
	ScoreData(Scorer& sc);
	
	~ScoreData(){};
		
	inline void clear() { array_.clear(); }
	
	inline ScoreArray get(int i){ return array_.at(i); }
	inline bool exists(int i){ return (i<array_.size())?true:false; }

	inline ScoreStats get(int i, int j){ return array_.at(i).get(j); }
	
	inline std::string name(){ return score_type; };

	void add(ScoreArray e){ array_.push_back(e); }
	void add(ScoreStats e, int sent_idx);
	
	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }

	void save(const std::string &file, bool bin=false);
	void save(ofstream& outFile, bool bin=false);
	inline void save(bool bin=false){ save("/dev/stdout", bin); }

	void load(ifstream& inFile);
	void load(const std::string &file);

	void loadnbest(const std::string &file);

};


#endif
