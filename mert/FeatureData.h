/*
 *  FeatureData.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_DATA_H
#define FEATURE_DATA_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"
#include "FeatureArray.h"

class FeatureData
{
protected:
	vector<FeatureArray> array_;
	vector<int> idxmap_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
	
public:
	FeatureData();
	
	~FeatureData(){};
		
	inline void clear() { array_.clear(); }
	
	inline FeatureArray& get(int i){ return array_.at(i); }
	inline const FeatureArray& get(int i)const{ return array_.at(i); }
	inline bool exists(int i){ return (i<array_.size())?true:false; }

	inline FeatureStats& get(int i, int j){ return array_.at(i).get(j); }
	inline const FeatureStats&  get(int i, int j)const{ return array_.at(i).get(j); }
	void add(FeatureArray e){ array_.push_back(e); }
	void add(FeatureStats e, int sent_idx);
	
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
