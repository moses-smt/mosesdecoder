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

class ScoreData
{
protected:
	vector<ScoreArray> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	
public:
	ScoreData();
	
	~ScoreData(){};
		
	inline void clear() { array_.clear(); }
	
	inline ScoreArray get(int i){ return array_.at(i); }
	inline bool exists(int i){ return (i<array_.size())?true:false; }

	inline ScoreStats get(int i, int j){ return array_.at(i).get(j); }

	void add(ScoreArray e){ array_.push_back(e); }
	void add(ScoreStats e, int sent_idx);
	
	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }

	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }

	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

	void loadnbest(const std::string &file);

};


#endif