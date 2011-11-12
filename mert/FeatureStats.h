/*
 *  FeatureStats.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef FEATURE_STATS_H
#define FEATURE_STATS_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"

#define FEATURE_STATS_MIN (numeric_limits<FeatureStatsType>::min())
#define ATOFST(str) ((FeatureStatsType) atof(str))

#define featbytes_ (entries_*sizeof(FeatureStatsType))

// Minimal sparse vector
class SparseVector {
public:
  typedef std::map<size_t,FeatureStatsType> fvector_t;
  typedef std::map<std::string, size_t> name2id_t;
  typedef std::vector<std::string> id2name_t;

  FeatureStatsType get(std::string name) const;
  FeatureStatsType get(size_t id) const;
  void set(std::string name, FeatureStatsType value);
  void clear();
  size_t size() const;

  void write(std::ostream& out, const std::string& sep = " ") const;

  SparseVector& operator-=(const SparseVector& rhs);

private:
  static name2id_t name2id_;
  static id2name_t id2name_;
  fvector_t fvector_;
};

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs);

class FeatureStats
{
private:
  featstats_t array_;
  SparseVector map_;
  size_t entries_;
  size_t available_;

public:
  FeatureStats();
  explicit FeatureStats(const size_t size);
  explicit FeatureStats(std::string &theString);

  // We intentionally allow copying.
  FeatureStats(const FeatureStats &stats);
  FeatureStats& operator=(const FeatureStats &stats);

  ~FeatureStats();

  bool isfull() {
    return (entries_ < available_)?0:1;
  }
  void expand();
  void add(FeatureStatsType v);
  void addSparse(string name, FeatureStatsType v);

  inline void clear() {
    memset((void*) array_,0,featbytes_);
    map_.clear();
  }

  inline FeatureStatsType get(size_t i) {
    return array_[i];
  }
  inline FeatureStatsType get(size_t i)const {
    return array_[i];
  }
  inline featstats_t getArray() const {
    return array_;
  }
  inline const SparseVector& getSparse() const {
    return map_;
  }

  void set(std::string &theString);

  inline size_t bytes() const {
    return featbytes_;
  }
  inline size_t size() const {
    return entries_;
  }
  inline size_t available() const {
    return available_;
  }

  void savetxt(const std::string &file);
  void savetxt(ofstream& outFile);
  void savebin(ofstream& outFile);
  inline void savetxt() {
    savetxt("/dev/stdout");
  }

  void loadtxt(const std::string &file);
  void loadtxt(ifstream& inFile);
  void loadbin(ifstream& inFile);

  inline void reset() {
    entries_ = 0;
    clear();
  }

  /**
   * Write the whole object to a stream.
   */
  friend ostream& operator<<(ostream& o, const FeatureStats& e);
};

#endif  // FEATURE_STATS_H
