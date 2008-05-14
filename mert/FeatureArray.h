/*
 *  FeatureArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_ARRAY_H
#define FEATURE_ARRAY_H

#define FEATURES_BEGIN "FEATURES_BEGIN_0"
#define FEATURES_END "FEATURES_END_0"

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
	size_t idx; // idx to identify the utterance 
	
public:
	FeatureArray();
	
	~FeatureArray(){};
		
	inline void clear() { array_.clear(); }
	
	inline size_t getIndex(){ return idx; }
	inline void setIndex(size_t value){ idx=value; }

	inline FeatureStats get(int i){ return array_.at(i); }
	void add(FeatureStats e){ array_.push_back(e); }
	
	inline size_t size(){ return array_.size(); }
	
	inline size_t memsize(){ return bufLen_; }
	

	void savetxt(const std::string &file);
	void savetxt(ofstream& outFile);
	inline void savetxt(){ savetxt("/dev/stdout"); }

	void loadtxt(ifstream& inFile);
	void loadtxt(const std::string &file);

};


#endif