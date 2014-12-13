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

#include <boost/unordered_map.hpp>
#include "util/string_piece.hh"
#include "Types.h"

namespace MosesTuning
{


// Minimal sparse vector
class SparseVector
{
public:
  typedef std::map<std::size_t,FeatureStatsType> fvector_t;
  typedef std::map<std::string, std::size_t> name2id_t;
  typedef std::vector<std::string> id2name_t;

  FeatureStatsType get(const std::string& name) const;
  FeatureStatsType get(std::size_t id) const;
  void set(const std::string& name, FeatureStatsType value);
  void set(size_t id, FeatureStatsType value);
  void clear();
  void load(const std::string& file);
  std::size_t size() const {
    return m_fvector.size();
  }

  void write(std::ostream& out, const std::string& sep = " ") const;

  SparseVector& operator-=(const SparseVector& rhs);
  SparseVector& operator+=(const SparseVector& rhs);
  FeatureStatsType inner_product(const SparseVector& rhs) const;

  // Added by cherryc
  std::vector<std::size_t> feats() const;
  friend bool operator==(SparseVector const& item1, SparseVector const& item2);
  friend std::size_t hash_value(SparseVector const& item);
  static std::size_t encode(const std::string& feat);
  static std::string decode(std::size_t feat);
  // End added by cherryc

private:
  static name2id_t m_name_to_id;
  static id2name_t m_id_to_name;
  fvector_t m_fvector;
};

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs);
FeatureStatsType inner_product(const SparseVector& lhs, const SparseVector& rhs);

class FeatureStats
{
private:
  std::size_t m_available_size;
  std::size_t m_entries;

  // TODO: Use smart pointer for exceptional-safety.
  featstats_t m_array;
  SparseVector m_map;

public:
  FeatureStats();
  explicit FeatureStats(const std::size_t size);

  ~FeatureStats();

  // We intentionally allow copying.
  FeatureStats(const FeatureStats &stats);
  FeatureStats& operator=(const FeatureStats &stats);

  void Copy(const FeatureStats &stats);

  bool isfull() const {
    return (m_entries < m_available_size) ? 0 : 1;
  }
  void expand();
  void add(FeatureStatsType v);
  void addSparse(const std::string& name, FeatureStatsType v);

  void clear() {
    memset((void*)m_array, 0, GetArraySizeWithBytes());
    m_map.clear();
  }

  void reset() {
    m_entries = 0;
    clear();
  }

  FeatureStatsType get(std::size_t i) {
    return m_array[i];
  }
  FeatureStatsType get(std::size_t i)const {
    return m_array[i];
  }
  featstats_t getArray() const {
    return m_array;
  }

  const SparseVector& getSparse() const {
    return m_map;
  }

  void set(std::string &theString, const SparseVector& sparseWeights);

  inline std::size_t bytes() const {
    return GetArraySizeWithBytes();
  }

  std::size_t GetArraySizeWithBytes() const {
    return m_entries * sizeof(FeatureStatsType);
  }

  std::size_t size() const {
    return m_entries;
  }

  std::size_t available() const {
    return m_available_size;
  }

  void savetxt(const std::string &file);
  void savetxt(std::ostream* os);
  void savebin(std::ostream* os);
  void savetxt();

  void loadtxt(std::istream* is, const SparseVector& sparseWeights);
  void loadbin(std::istream* is);

  /**
   * Write the whole object to a stream.
   */
  friend std::ostream& operator<<(std::ostream& o, const FeatureStats& e);
};

bool operator==(const FeatureStats& f1, const FeatureStats& f2);

}

#endif  // MERT_FEATURE_STATS_H_
