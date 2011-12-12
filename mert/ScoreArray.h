/*
 *  ScoreArray.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_ARRAY_H
#define SCORE_ARRAY_H

using namespace std;

#include <vector>
#include <iostream>
#include <string>

#include "ScoreStats.h"

const char SCORES_TXT_BEGIN[] = "SCORES_TXT_BEGIN_0";
const char SCORES_TXT_END[] = "SCORES_TXT_END_0";
const char SCORES_BIN_BEGIN[] = "SCORES_BIN_BEGIN_0";
const char SCORES_BIN_END[] = "SCORES_BIN_END_0";

class ScoreArray
{
protected:
  scorearray_t array_;
  std::string score_type;
  size_t number_of_scores;

private:
  // idx to identify the utterance.
  // It can differ from the index inside the vector.
  std::string  idx;

public:
  ScoreArray();
  ~ScoreArray() {}

  inline void clear() {
    array_.clear();
  }

  inline std::string getIndex() const {
    return idx;
  }
  inline void setIndex(const std::string& value) {
    idx=value;
  }

//	inline ScoreStats get(size_t i){ return array_.at(i); }

  inline ScoreStats&  get(size_t i) {
    return array_.at(i);
  }
  inline const ScoreStats&  get(size_t i)const {
    return array_.at(i);
  }

  void add(const ScoreStats& e) {
    array_.push_back(e);
  }

  //ADDED BY TS
  void swap(size_t i, size_t j) {
    std::swap(array_[i],array_[j]);
  }

  void resize(size_t new_size) {
    array_.resize(std::min(new_size,array_.size()));
  }
  //END_ADDED

  void merge(ScoreArray& e);

  inline std::string name() const {
    return score_type;
  }

  inline void name(std::string &sctype) {
    score_type = sctype;
  }

  inline size_t size() const {
    return array_.size();
  }
  inline size_t NumberOfScores() const {
    return number_of_scores;
  }
  inline void NumberOfScores(size_t v) {
    number_of_scores = v;
  }

  void savetxt(ofstream& outFile, const std::string& sctype);
  void savebin(ofstream& outFile, const std::string& sctype);
  void save(ofstream& outFile, const std::string& sctype, bool bin=false);
  void save(const std::string &file, const std::string& sctype, bool bin=false);
  inline void save(const std::string& sctype, bool bin=false) {
    save("/dev/stdout", sctype, bin);
  }

  void loadtxt(ifstream& inFile, size_t n);
  void loadbin(ifstream& inFile, size_t n);
  void load(ifstream& inFile);
  void load(const std::string &file);

  bool check_consistency() const;
};

#endif  // SCORE_ARRAY_H
