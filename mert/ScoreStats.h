/*
 *  ScoreStats.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_STATS_H
#define SCORE_STATS_H

#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include "Types.h"

using namespace std;

class ScoreStats
{
private:
  size_t available_;
  size_t entries_;

  // TODO: Use smart pointer for exceptional-safety.
  scorestats_t array_;

public:
  ScoreStats();
  explicit ScoreStats(const size_t size);
  explicit ScoreStats(std::string &theString);
  ~ScoreStats();

  // We intentionally allow copying.
  ScoreStats(const ScoreStats &stats);
  ScoreStats& operator=(const ScoreStats &stats);

  void Copy(const ScoreStats &stats);

  bool isfull() const {
    return (entries_ < available_) ? 0 : 1;
  }

  void expand();
  void add(ScoreStatsType v);

  void clear() {
    memset((void*)array_, 0, GetArraySizeWithBytes());
  }

  void reset() {
    entries_ = 0;
    clear();
  }

  inline ScoreStatsType get(size_t i) {
    return array_[i];
  }
  inline ScoreStatsType get(size_t i)const {
    return array_[i];
  }
  inline scorestats_t getArray() const {
    return array_;
  }

  void set(std::string &theString);

  inline size_t bytes() const {
    return GetArraySizeWithBytes();
  }

  size_t GetArraySizeWithBytes() const {
    return entries_ * sizeof(ScoreStatsType);
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

  /**
   * Write the whole object to a stream.
   */
  friend ostream& operator<<(ostream& o, const ScoreStats& e);
};

//ADDED_BY_TS
bool operator==(const ScoreStats& s1, const ScoreStats& s2); 
//END_ADDED

#endif  // SCORE_STATS_H
