/*
 *  ScoreStats.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_SCORE_STATS_H_
#define MERT_SCORE_STATS_H_

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
  size_t m_available_size;
  size_t m_entries;

  // TODO: Use smart pointer for exceptional-safety.
  scorestats_t m_array;

public:
  ScoreStats();
  explicit ScoreStats(const size_t size);

  ~ScoreStats();

  // We intentionally allow copying.
  ScoreStats(const ScoreStats &stats);
  ScoreStats& operator=(const ScoreStats &stats);

  void Copy(const ScoreStats &stats);

  bool isfull() const {
    return (m_entries < m_available_size) ? 0 : 1;
  }

  void expand();
  void add(ScoreStatsType v);

  void clear() {
    memset((void*)m_array, 0, GetArraySizeWithBytes());
  }

  void reset() {
    m_entries = 0;
    clear();
  }

  inline ScoreStatsType get(size_t i) { return m_array[i]; }
  inline ScoreStatsType get(size_t i) const { return m_array[i]; }
  inline scorestats_t getArray() const { return m_array; }

  void set(const std::string& str);

  // Much more efficient than the above.
  void set(const std::vector<ScoreStatsType>& stats) {
    reset();
    for (size_t i = 0; i < stats.size(); ++i) {
      add(stats[i]);
    }
  }

  inline size_t bytes() const { return GetArraySizeWithBytes(); }

  size_t GetArraySizeWithBytes() const {
    return m_entries * sizeof(ScoreStatsType);
  }

  inline size_t size() const { return m_entries; }

  inline size_t available() const { return m_available_size; }

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

#endif  // MERT_SCORE_STATS_H_
