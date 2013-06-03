/*
 *  ScoreData.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_SCORE_DATA_H_
#define MERT_SCORE_DATA_H_

#include <iosfwd>
#include <vector>
#include <stdexcept>
#include <string>
#include "ScoreArray.h"
#include "ScoreStats.h"

namespace MosesTuning
{


class Scorer;

class ScoreData
{
private:
  // Do not allow the user to instanciate without arguments.
  ScoreData() {}

  scoredata_t m_array;
  idx2name m_index_to_array_name; // map from index to name of array
  name2idx m_array_name_to_index; // map from name to index of array

  Scorer* m_scorer;
  std::string m_score_type;
  std::size_t m_num_scores;

public:
  ScoreData(Scorer* scorer);
  ~ScoreData() {}

  void clear() {
    m_array.clear();
  }

  inline ScoreArray& get(std::size_t idx) {
    return m_array.at(idx);
  }

  inline const ScoreArray& get(std::size_t idx) const {
    return m_array.at(idx);
  }

  inline bool exists(int sent_idx) const {
    return existsInternal(getIndex(sent_idx));
  }

  inline bool existsInternal(int sent_idx) const {
    return (sent_idx > -1 && sent_idx < static_cast<int>(m_array.size())) ? true : false;
  }

  inline ScoreStats& get(std::size_t i, std::size_t j) {
    return m_array.at(i).get(j);
  }

  inline const ScoreStats& get(std::size_t i, std::size_t j) const {
    return m_array.at(i).get(j);
  }

  std::string name() const {
    return m_score_type;
  }

  std::string name(const std::string &score_type) {
    return m_score_type = score_type;
  }

  void add(ScoreArray& e);
  void add(const ScoreStats& e, int sent_idx);

  std::size_t NumberOfScores() const {
    return m_num_scores;
  }
  std::size_t size() const {
    return m_array.size();
  }

  void save(const std::string &file, bool bin=false);
  void save(std::ostream* os, bool bin=false);
  void save(bool bin=false);

  void load(std::istream* is);
  void load(const std::string &file);

  bool check_consistency() const;

  void setIndex();

  inline int getIndex(const int idx) const {
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
};

}

#endif  // MERT_SCORE_DATA_H_
