/*
 *  FeatureArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_ARRAY_H
#define FEATURE_ARRAY_H

#include <vector>
#include <iostream>
#include <fstream>
#include "FeatureStats.h"

using namespace std;

const char FEATURES_TXT_BEGIN[] = "FEATURES_TXT_BEGIN_0";
const char FEATURES_TXT_END[] = "FEATURES_TXT_END_0";
const char FEATURES_BIN_BEGIN[] = "FEATURES_BIN_BEGIN_0";
const char FEATURES_BIN_END[] = "FEATURES_BIN_END_0";

class FeatureArray
{
private:
  // idx to identify the utterance. It can differ from
  // the index inside the vector.
  std::string idx;

protected:
  featarray_t array_;
  size_t number_of_features;
  std::string features;
  bool _sparse_flag;

public:
  FeatureArray();
  ~FeatureArray();

  inline void clear() {
    array_.clear();
  }

  inline bool hasSparseFeatures() const {
    return _sparse_flag;
  }

  inline std::string getIndex() const {
    return idx;
  }
  inline void setIndex(const std::string& value) {
    idx = value;
  }

  inline FeatureStats& get(size_t i) {
    return array_.at(i);
  }
  inline const FeatureStats& get(size_t i)const {
    return array_.at(i);
  }
  void add(FeatureStats& e) {
    array_.push_back(e);
  }

  //ADDED BY TS
  void swap(size_t i, size_t j) {
    std::swap(array_[i],array_[j]);
  }
  
  void resize(size_t new_size) {
    array_.resize(std::min(new_size,array_.size()));
  }
  //END_ADDED

  void merge(FeatureArray& e);

  inline size_t size() const {
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
  inline void Features(const std::string& f) {
    features = f;
  }

  void savetxt(ofstream& outFile);
  void savebin(ofstream& outFile);
  void save(ofstream& outFile, bool bin=false);
  void save(const std::string &file, bool bin=false);
  inline void save(bool bin=false) {
    save("/dev/stdout",bin);
  }

  void loadtxt(ifstream& inFile, size_t n);
  void loadbin(ifstream& inFile, size_t n);
  void load(ifstream& inFile);
  void load(const std::string &file);

  bool check_consistency() const;
};

#endif  // FEATURE_ARRAY_H
