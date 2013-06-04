/*
 *  FeatureArray.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_FEATURE_ARRAY_H_
#define MERT_FEATURE_ARRAY_H_

#include <vector>
#include <iosfwd>
#include "FeatureStats.h"

namespace MosesTuning
{


const char FEATURES_TXT_BEGIN[] = "FEATURES_TXT_BEGIN_0";
const char FEATURES_TXT_END[] = "FEATURES_TXT_END_0";
const char FEATURES_BIN_BEGIN[] = "FEATURES_BIN_BEGIN_0";
const char FEATURES_BIN_END[] = "FEATURES_BIN_END_0";

class FeatureArray
{
private:
  // idx to identify the utterance. It can differ from
  // the index inside the vector.
  int m_index;
  featarray_t m_array;
  std::size_t m_num_features;
  std::string m_features;

public:
  FeatureArray();
  ~FeatureArray();

  void clear() {
    m_array.clear();
  }


  int getIndex() const {
    return m_index;
  }
  void setIndex(const int value) {
    m_index = value;
  }

  FeatureStats& get(std::size_t i) {
    return m_array.at(i);
  }
  const FeatureStats& get(std::size_t i) const {
    return m_array.at(i);
  }

  void add(FeatureStats& e) {
    m_array.push_back(e);
  }

  //ADDED BY TS
  void swap(std::size_t i, std::size_t j) {
    std::swap(m_array[i], m_array[j]);
  }

  void resize(std::size_t new_size) {
    m_array.resize(std::min(new_size, m_array.size()));
  }
  //END_ADDED

  void merge(FeatureArray& e);

  std::size_t size() const {
    return m_array.size();
  }

  std::size_t NumberOfFeatures() const {
    return m_num_features;
  }
  void NumberOfFeatures(std::size_t v) {
    m_num_features = v;
  }

  std::string Features() const {
    return m_features;
  }
  void Features(const std::string& f) {
    m_features = f;
  }

  void savetxt(std::ostream* os);
  void savebin(std::ostream* os);
  void save(std::ostream* os, bool bin=false);
  void save(const std::string &file, bool bin=false);
  void save(bool bin=false);

  void loadtxt(std::istream* is, const SparseVector& sparseWeights, std::size_t n);
  void loadbin(std::istream* is, std::size_t n);
  void load(std::istream* is, const SparseVector& sparseWeights);

  bool check_consistency() const;
};

}

#endif  // MERT_FEATURE_ARRAY_H_
