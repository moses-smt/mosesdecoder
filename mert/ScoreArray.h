/*
 *  ScoreArray.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_SCORE_ARRAY_H_
#define MERT_SCORE_ARRAY_H_

#include <vector>
#include <iostream>
#include <string>

#include "ScoreStats.h"

namespace MosesTuning
{

const char SCORES_TXT_BEGIN[] = "SCORES_TXT_BEGIN_0";
const char SCORES_TXT_END[] = "SCORES_TXT_END_0";
const char SCORES_BIN_BEGIN[] = "SCORES_BIN_BEGIN_0";
const char SCORES_BIN_END[] = "SCORES_BIN_END_0";

class ScoreArray
{
private:
  scorearray_t m_array;
  std::string m_score_type;
  std::size_t m_num_scores;

  // indexx to identify the utterance.
  // It can differ from the index inside the vector.
  int m_index;

public:
  ScoreArray();
  ~ScoreArray() {}

  void clear() {
    m_array.clear();
  }

  int getIndex() const {
    return m_index;
  }

  void setIndex(int value) {
    m_index = value;
  }

  ScoreStats& get(std::size_t i) {
    return m_array.at(i);
  }

  const ScoreStats& get(std::size_t i) const {
    return m_array.at(i);
  }

  void add(const ScoreStats& e) {
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

  void merge(ScoreArray& e);

  std::string name() const {
    return m_score_type;
  }

  void name(std::string &score_type) {
    m_score_type = score_type;
  }

  std::size_t size() const {
    return m_array.size();
  }

  std::size_t NumberOfScores() const {
    return m_num_scores;
  }

  void NumberOfScores(std::size_t v) {
    m_num_scores = v;
  }

  void savetxt(std::ostream* os, const std::string& score_type);
  void savebin(std::ostream* os, const std::string& score_type);
  void save(std::ostream* os, const std::string& score_type, bool bin=false);
  void save(const std::string &file, const std::string& score_type, bool bin=false);
  void save(const std::string& score_type, bool bin=false);

  void loadtxt(std::istream* is, std::size_t n);
  void loadbin(std::istream* is, std::size_t n);
  void load(std::istream* is);
  void load(const std::string &file);

  bool check_consistency() const;
};

}

#endif  // MERT_SCORE_ARRAY_H_
