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
#include <iosfwd>
#include <cstdlib>
#include <cstring>

#include "Types.h"

namespace MosesTuning
{


class ScoreStats
{
private:
  std::size_t m_available_size;
  std::size_t m_entries;

  // TODO: Use smart pointer for exceptional-safety.
  scorestats_t m_array;

public:
  ScoreStats();
  explicit ScoreStats(const std::size_t size);

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
    std::memset((void*)m_array, 0, GetArraySizeWithBytes());
  }

  void reset() {
    m_entries = 0;
    clear();
  }

  ScoreStatsType get(std::size_t i) {
    return m_array[i];
  }
  ScoreStatsType get(std::size_t i) const {
    return m_array[i];
  }
  scorestats_t getArray() const {
    return m_array;
  }

  void set(const std::string& str);

  // Much more efficient than the above.
  void set(const std::vector<ScoreStatsType>& stats) {
    reset();
    for (std::size_t i = 0; i < stats.size(); ++i) {
      add(stats[i]);
    }
  }

  std::size_t bytes() const {
    return GetArraySizeWithBytes();
  }

  std::size_t GetArraySizeWithBytes() const {
    return m_entries * sizeof(ScoreStatsType);
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

  void loadtxt(const std::string &file);
  void loadtxt(std::istream* is);
  void loadbin(std::istream* is);

  /**
   * Write the whole object to a stream.
   */
  friend std::ostream& operator<<(std::ostream& o, const ScoreStats& e);
};

bool operator==(const ScoreStats& s1, const ScoreStats& s2);

}

#endif  // MERT_SCORE_STATS_H_
