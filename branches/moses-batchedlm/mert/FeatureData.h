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
	featdata_t array_;
	idx2name idx2arrayname_; //map from index to name of array
	name2idx arrayname2idx_; //map from name to index of array
	
	
private:
	size_t number_of_features;
	std::string features;

	map<std::string, size_t> featname2idx_; //map from name to index of features
	map<size_t, std::string> idx2featname_; //map from index to name of features
	
public:
	FeatureData();
	
	~FeatureData(){};
		
	inline void clear() { array_.clear(); }
	
	inline FeatureArray get(const std::string& idx){ return array_.at(getIndex(idx)); }
	inline FeatureArray& get(size_t idx){ return array_.at(idx); }
	inline const FeatureArray& get(size_t idx) const{ return array_.at(idx); }

	inline bool exists(const std::string & sent_idx){	return exists(getIndex(sent_idx)); }
	inline bool exists(int sent_idx){ return (sent_idx>-1 && sent_idx<(int) array_.size())?true:false; }

	inline FeatureStats& get(size_t i, size_t j){ return array_.at(i).get(j); }
	inline const FeatureStats&  get(size_t i, size_t j) const { return array_.at(i).get(j); }
	
	void add(FeatureArray& e);
	void add(FeatureStats& e, const std::string& sent_idx);
	
	inline size_t size(){ return array_.size(); }
	inline size_t NumberOfFeatures() const{	return number_of_features; }
	inline void NumberOfFeatures(size_t v){	number_of_features = v;	}
	inline std::string Features() const{ return features; }
	inline void Features(const std::string f){ features = f; }
	
	void save(const std::string &file, bool bin=false);
	void save(ofstream& outFile, bool bin=false);
	inline void save(bool bin=false){ save("/dev/stdout", bin); }

	void load(ifstream& inFile);
	void load(const std::string &file);
	
	bool check_consistency();
	void setIndex();
	
	inline int getIndex(const std::string& idx){
		name2idx::iterator i = arrayname2idx_.find(idx);
		if (i!=arrayname2idx_.end())
			return i->second;
		else
			return -1;		
  }
	
  inline std::string getIndex(size_t idx){
		idx2name::iterator i = idx2arrayname_.find(idx);
		if (i!=idx2arrayname_.end())
			throw runtime_error("there is no entry at index " + idx);
		return i->second;
	}
	
	
	bool existsFeatureNames(){ return (idx2featname_.size() > 0)?true:false; };
	
	std::string getFeatureName(size_t idx){
		if (idx >= idx2featname_.size())
			throw runtime_error("Error: you required an too big index");
		return idx2featname_[idx];
	};
	
  size_t getFeatureIndex(const std::string& name){
		if (featname2idx_.find(name)==featname2idx_.end())
			throw runtime_error("Error: feature " + name +" is unknown");
		return featname2idx_[name];
	};
	
  void setFeatureMap(const std::string feat);
};


#endif

