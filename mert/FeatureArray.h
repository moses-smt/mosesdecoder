/*
 *  FeatureArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_ARRAY_H
#define FEATURE_ARRAY_H

#define FEATURES_TXT_BEGIN "FEATURES_TXTB_BEGIN_0"
#define FEATURES_TXT_END "FEATURES_TXT_END_0"
#define FEATURES_BIN_BEGIN "FEATURES_BIN_BEGIN_0"
#define FEATURES_BIN_END "FEATURES_BIN_END_0"

using namespace std;

#include <limits>
#include <vector>
#include <iostream>
#include <fstream>

#include "Util.h"
#include "FeatureStats.h"

class FeatureArray
{
protected:
	vector<FeatureStats> array_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	size_t idx; // idx to identify the utterance, it can differ from the index inside the vector
	
public:
	FeatureArray();
	
	~FeatureArray(){};
		
	inline void clear() { array_.clear(); }
	
	inline size_t getIndex(){ return idx; }
	inline void setIndex(size_t value){ idx=value; }

	inline FeatureStats get(int i){ return array_.at(i); }
	inline FeatureStats get(int i)const{ return array_.at(i); }
	void add(FeatureStats e){ array_.push_back(e); }
	
	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }
	

	void savetxt(ofstream& outFile);
	void savebin(ofstream& outFile);
	void save(ofstream& outFile, bool bin=false);
	void save(const std::string &file, bool bin=false);
	inline void save(bool bin=false){ save("/dev/stdout",bin); }

	void loadtxt(ifstream& inFile);
	void loadbin(ifstream& inFile);
	void load(ifstream& inFile, bool bin=false);
	void load(const std::string &file, bool bin=false);

};


#endif
