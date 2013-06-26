/*
 *  FeatureData.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_FEATURE_DATA_H_
#define MERT_FEATURE_DATA_H_

#include <vector>
#include <iostream>
#include <stdexcept>
#include "FeatureArray.h"

namespace MosesTuning
{


class FeatureData
{
private:
  std::size_t m_num_features;
  std::string m_features;
  std::map<std::string, std::size_t> m_feature_name_to_index; // map from name to index of features
  std::map<std::size_t, std::string> m_index_to_feature_name; // map from index to name of features
  featdata_t m_array;
  idx2name m_index_to_array_name; // map from index to name of array
  name2idx m_array_name_to_index; // map from name to index of array

public:
  FeatureData();
  ~FeatureData() {}

  void clear() {
    m_array.clear();
  }

  FeatureArray& get(size_t idx) {
    return m_array.at(idx);
  }
  const FeatureArray& get(size_t idx) const {
    return m_array.at(idx);
  }

  inline bool exists(int sent_idx) const {
    return existsInternal(getIndex(sent_idx));
  }

  inline bool existsInternal(int sent_idx) const {
    return (sent_idx > -1 && sent_idx < static_cast<int>(m_array.size())) ? true : false;
  }

  inline FeatureStats& get(std::size_t i, std::size_t j) {
    return m_array.at(i).get(j);
  }

  inline const FeatureStats& get(std::size_t i, std::size_t j) const {
    return m_array.at(i).get(j);
  }

  void add(FeatureArray& e);
  void add(FeatureStats& e, int sent_idx);

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

  void save(const std::string &file, bool bin=false);
  void save(std::ostream* os, bool bin=false);
  void save(bool bin=false);

  void load(std::istream* is, const SparseVector& sparseWeights);
  void load(const std::string &file, const SparseVector& sparseWeights);

  bool check_consistency() const;

  void setIndex();

  inline int getIndex(int idx) const {
    name2idx::const_iterator i = m_array_name_to_index.find(idx);
    if (i != m_array_name_to_index.end())
      return i->second;
    else
      return -1;
  }

  inline int getName(std::size_t idx) const {
    idx2name::const_iterator i = m_index_to_array_name.find(idx);
    if (i != m_index_to_array_name.end())
      throw std::runtime_error("there is no entry at index " + idx);
    return i->second;
  }

  bool existsFeatureNames() const {
    return (m_index_to_feature_name.size() > 0) ? true : false;
  }

  std::string getFeatureName(std::size_t idx) const {
    if (idx >= m_index_to_feature_name.size())
      throw std::runtime_error("Error: you required an too big index");
    std::map<std::size_t, std::string>::const_iterator it = m_index_to_feature_name.find(idx);
    if (it == m_index_to_feature_name.end()) {
      throw std::runtime_error("Error: specified id is unknown: " + idx);
    } else {
      return it->second;
    }
  }

  std::size_t getFeatureIndex(const std::string& name) const {
    std::map<std::string, std::size_t>::const_iterator it = m_feature_name_to_index.find(name);
    if (it == m_feature_name_to_index.end()) {
      std::string msg = "Error: feature " + name + " is unknown. Known features: ";
      for (std::map<std::string, std::size_t>::const_iterator cit = m_feature_name_to_index.begin();
           cit != m_feature_name_to_index.end(); cit++) {
        msg += cit->first;
        msg += ", ";
      }

      throw std::runtime_error(msg);
    }
    return it->second;
  }

  void setFeatureMap(const std::string& feat);

  /* For debugging */
  std::string ToString() const;
};

}

#endif  // MERT_FEATURE_DATA_H_
