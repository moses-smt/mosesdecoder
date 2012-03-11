/*
 *  FeatureStats.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_FEATURE_STATS_H_
#define MERT_FEATURE_STATS_H_

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Types.h"

using namespace std;

// Minimal sparse vector
class SparseVector {
public:
  typedef std::map<size_t,FeatureStatsType> fvector_t;
  typedef std::map<std::string, size_t> name2id_t;
  typedef std::vector<std::string> id2name_t;

  FeatureStatsType get(const std::string& name) const;
  FeatureStatsType get(size_t id) const;
  void set(const std::string& name, FeatureStatsType value);
  void clear();
  size_t size() const { return m_fvector.size(); }

  void write(std::ostream& out, const std::string& sep = " ") const;

  SparseVector& operator-=(const SparseVector& rhs);

private:
  static name2id_t m_name_to_id;
  static id2name_t m_id_to_name;
  fvector_t m_fvector;
};

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs);

class FeatureStats
{
private:
  size_t m_available_size;
  size_t m_entries;

  // TODO: Use smart pointer for exceptional-safety.
  featstats_t m_array;
  SparseVector m_map;

public:
  FeatureStats();
  explicit FeatureStats(const size_t size);
  explicit FeatureStats(std::string &theString);

  ~FeatureStats();

  // We intentionally allow copying.
  FeatureStats(const FeatureStats &stats);
  FeatureStats& operator=(const FeatureStats &stats);

  void Copy(const FeatureStats &stats);

  bool isfull() const { return (m_entries < m_available_size) ? 0 : 1; }
  void expand();
  void add(FeatureStatsType v);
  void addSparse(const string& name, FeatureStatsType v);

  void clear() {
    memset((void*)m_array, 0, GetArraySizeWithBytes());
    m_map.clear();
  }

  void reset() {
    m_entries = 0;
    clear();
  }

  FeatureStatsType get(size_t i) { return m_array[i]; }
  FeatureStatsType get(size_t i)const { return m_array[i]; }
  featstats_t getArray() const { return m_array; }

  const SparseVector& getSparse() const { return m_map; }

  void set(std::string &theString);

  inline size_t bytes() const { return GetArraySizeWithBytes(); }

  size_t GetArraySizeWithBytes() const {
    return m_entries * sizeof(FeatureStatsType);
  }

  size_t size() const { return m_entries; }

  size_t available() const { return m_available_size; }

  void savetxt(const std::string &file);
  void savetxt(std::ostream* os);
  void savebin(std::ostream* os);
  void savetxt();

  void loadtxt(const std::string &file);
  void loadtxt(std::istream* is);
  void loadbin(std::istream* is);

  /**
   * Write the whole object to a stream.
   */
  friend ostream& operator<<(ostream& o, const FeatureStats& e);
};

//ADEED_BY_TS
bool operator==(const FeatureStats& f1, const FeatureStats& f2);
//END_ADDED

#endif  // MERT_FEATURE_STATS_H_
