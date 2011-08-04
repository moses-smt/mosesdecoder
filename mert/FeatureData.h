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

  ~FeatureData() {};

  inline void clear() {
    array_.clear();
  }

  inline FeatureArray get(const std::string& idx) {
    return array_.at(getIndex(idx));
  }
  inline FeatureArray& get(size_t idx) {
    return array_.at(idx);
  }
  inline const FeatureArray& get(size_t idx) const {
    return array_.at(idx);
  }
  inline void set(FeatureArray& e , size_t idx) {
    array_.at(idx)=e;
  }
// 	inline void add(FeatureArray&){array_.push_back(FeatureArray);}

  inline bool exists(const std::string & sent_idx) {
    return exists(getIndex(sent_idx));
  }
  inline bool exists(int sent_idx) {
    return (sent_idx>-1 && sent_idx<(int) array_.size())?true:false;
  }

  inline FeatureStats& get(size_t i, size_t j) {
    return array_.at(i).get(j);
  }
  inline const FeatureStats&  get(size_t i, size_t j) const {
    return array_.at(i).get(j);
  }

  void add(FeatureArray& e);
  void add(FeatureArray& e, float l_lambda);
  void add(FeatureStats& e, const std::string& sent_idx);

  inline size_t size() {
    return array_.size();
  }
  inline size_t NumberOfFeatures() const {
    return number_of_features;
  }
  inline void NumberOfFeatures(size_t v) {
    number_of_features = v;
  }
  inline std::string Features() const {
    return features;
  }
  inline void Features(const std::string f) {
    features = f;
  }

  void save(const std::string &file, bool bin=false);
  void save(ofstream& outFile, bool bin=false);
  inline void save(bool bin=false) {
    save("/dev/stdout", bin);
  }

  void load(ifstream& inFile);
  void load(const std::string &file);
  void applyLambda(float f);

  bool check_consistency();
  void setIndex();
  void mergeWithOtherData(FeatureData * feat);
  inline FeatureData operator+(const FeatureData& feat)const {
    FeatureData F;
    F.array_=array_;
    F.idx2arrayname_=idx2arrayname_;
    F.arrayname2idx_=arrayname2idx_;
    F.number_of_features=number_of_features;
    F.features=features;
    F.featname2idx_=featname2idx_;
    F.idx2featname_=idx2featname_;
    if (array_.size()!=feat.array_.size()) {
      cerr << "ERROR : FeatureData::operator+ : different size "<<endl;
      exit(0);
    }
    for (int i =0; i< (int)array_.size(); i++) {
      F.array_.at(i)=F.array_.at(i)+feat.array_.at(i);
    }
// 	F.features=features;
// 	F.idx=idx;
// 	F.number_of_features=number_of_features;
    return F;
  }
  inline FeatureData * operator+(FeatureData * feat) {
    FeatureData * F;
    F->array_=array_;
    F->idx2arrayname_=idx2arrayname_;
    F->arrayname2idx_=arrayname2idx_;
    F->number_of_features=number_of_features;
    F->features=features;
    F->featname2idx_=featname2idx_;
    F->idx2featname_=idx2featname_;
    if (array_.size()!=feat->array_.size()) {
      cerr << "ERROR : FeatureData::operator+ : different size "<<endl;
      exit(0);
    }
    for (int i =0; i< (int)array_.size(); i++) {
      F->array_.at(i)=F->array_.at(i)+feat->array_.at(i);
    }
// 	F.features=features;
// 	F.idx=idx;
// 	F.number_of_features=number_of_features;
    return F;
  }
  inline FeatureData * operator+(FeatureData feat) {
    FeatureData * F;
    F->array_=array_;
    F->idx2arrayname_=idx2arrayname_;
    F->arrayname2idx_=arrayname2idx_;
    F->number_of_features=number_of_features;
    F->features=features;
    F->featname2idx_=featname2idx_;
    F->idx2featname_=idx2featname_;
    if (array_.size()!=feat.array_.size()) {
      cerr << "ERROR : FeatureData::operator+ : different size "<<endl;
      exit(0);
    }
    for (int i =0; i< (int)array_.size(); i++) {
      F->array_.at(i)=F->array_.at(i)+feat.array_.at(i);
    }
// 	F.features=features;
// 	F.idx=idx;
// 	F.number_of_features=number_of_features;
    return F;
  }

  inline int getIndex(const std::string& idx) {
    name2idx::iterator i = arrayname2idx_.find(idx);
    if (i!=arrayname2idx_.end())
      return i->second;
    else
      return -1;
  }

  inline std::string getIndex(size_t idx) {
    idx2name::iterator i = idx2arrayname_.find(idx);
    if (i!=idx2arrayname_.end())
      throw runtime_error("there is no entry at index " + idx);
    return i->second;
  }


  bool existsFeatureNames() {
    return (idx2featname_.size() > 0)?true:false;
  };

  std::string getFeatureName(size_t idx) {
    if (idx >= idx2featname_.size())
      throw runtime_error("Error: you required an too big index");
    return idx2featname_[idx];
  };

  size_t getFeatureIndex(const std::string& name) {
    if (featname2idx_.find(name)==featname2idx_.end())
      throw runtime_error("Error: feature " + name +" is unknown");
    return featname2idx_[name];
  };

  void setFeatureMap(const std::string feat);
};


#endif

