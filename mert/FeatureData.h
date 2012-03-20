/*
 *  FeatureData.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_FEATURE_DATA_H_
#define MERT_FEATURE_DATA_H_

using namespace std;

#include <vector>
#include <iostream>
#include <stdexcept>
#include "FeatureArray.h"

class FeatureData
{
private:
  size_t m_num_features;
  std::string m_features;
  bool m_sparse_flag;
  map<std::string, size_t> m_feature_name_to_index; // map from name to index of features
  map<size_t, std::string> m_index_to_feature_name; // map from index to name of features
  featdata_t m_array;
  idx2name m_index_to_array_name; // map from index to name of array
  name2idx m_array_name_to_index; // map from name to index of array

public:
  FeatureData();
  ~FeatureData() {}

  void clear() { m_array.clear(); }

  bool hasSparseFeatures() const { return m_sparse_flag; }

  FeatureArray get(const std::string& idx) {
    return m_array.at(getIndex(idx));
  }

  FeatureArray& get(size_t idx) { return m_array.at(idx); }
  const FeatureArray& get(size_t idx) const { return m_array.at(idx); }

  inline bool exists(const std::string& sent_idx) const {
    return exists(getIndex(sent_idx));
  }

  inline bool exists(int sent_idx) const {
    return (sent_idx > -1 && sent_idx < static_cast<int>(m_array.size())) ? true : false;
  }

  inline FeatureStats& get(size_t i, size_t j) {
    return m_array.at(i).get(j);
  }

  inline const FeatureStats& get(size_t i, size_t j) const {
    return m_array.at(i).get(j);
  }

  void add(FeatureArray& e);
  void add(FeatureStats& e, const std::string& sent_idx);

  size_t size() const { return m_array.size(); }

  size_t NumberOfFeatures() const { return m_num_features; }
  void NumberOfFeatures(size_t v) { m_num_features = v; }

  std::string Features() const { return m_features; }
  void Features(const std::string& f) { m_features = f; }

  void save(const std::string &file, bool bin=false);
  void save(std::ostream* os, bool bin=false);
  void save(bool bin=false);

  void load(std::istream* is);
  void load(const std::string &file);

  bool check_consistency() const;

  void setIndex();

  inline int getIndex(const std::string& idx) const {
    name2idx::const_iterator i = m_array_name_to_index.find(idx);
    if (i != m_array_name_to_index.end())
      return i->second;
    else
      return -1;
  }

  inline std::string getIndex(size_t idx) const {
    idx2name::const_iterator i = m_index_to_array_name.find(idx);
    if (i != m_index_to_array_name.end())
      throw runtime_error("there is no entry at index " + idx);
    return i->second;
  }

  bool existsFeatureNames() const {
    return (m_index_to_feature_name.size() > 0) ? true : false;
  }

  std::string getFeatureName(size_t idx) const {
    if (idx >= m_index_to_feature_name.size())
      throw runtime_error("Error: you required an too big index");
    map<size_t, std::string>::const_iterator it = m_index_to_feature_name.find(idx);
    if (it == m_index_to_feature_name.end()) {
      throw runtime_error("Error: specified id is unknown: " + idx);
    } else {
      return it->second;
    }
  }

  size_t getFeatureIndex(const std::string& name) const {
    map<std::string, size_t>::const_iterator it = m_feature_name_to_index.find(name);
    if (it == m_feature_name_to_index.end())
      throw runtime_error("Error: feature " + name + " is unknown");
    return it->second;
  }

  void setFeatureMap(const std::string& feat);

  /* For debugging */
  std::string ToString() const;
};

#endif  // MERT_FEATURE_DATA_H_
