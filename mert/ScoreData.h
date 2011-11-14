/*
 *  ScoreData.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef SCORE_DATA_H
#define SCORE_DATA_H

#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>
#include "ScoreArray.h"
#include "ScoreStats.h"

using namespace std;

class Scorer;

class ScoreData
{
protected:
  scoredata_t array_;
  idx2name idx2arrayname_; // map from index to name of array
  name2idx arrayname2idx_; // map from name to index of array

private:
  // Do not allow the user to instanciate without arguments.
  ScoreData() {}

  Scorer* theScorer;
  std::string score_type;
  size_t number_of_scores;

public:
  ScoreData(Scorer& sc);
  ~ScoreData() {}

  inline void clear() {
    array_.clear();
  }

  inline ScoreArray get(const std::string& idx) {
    return array_.at(getIndex(idx));
  }
  inline ScoreArray& get(size_t idx) {
    return array_.at(idx);
  }
  inline const ScoreArray& get(size_t idx) const {
    return array_.at(idx);
  }

  inline bool exists(const std::string& sent_idx) const {
    return exists(getIndex(sent_idx));
  }
  inline bool exists(int sent_idx) const {
    return (sent_idx > -1 && sent_idx < (int)array_.size()) ? true : false;
  }

  inline ScoreStats& get(size_t i, size_t j) {
    return array_.at(i).get(j);
  }
  inline const ScoreStats&  get(size_t i, size_t j) const {
    return array_.at(i).get(j);
  }

  inline std::string name() const {
    return score_type;
  }

  inline std::string name(const std::string &sctype) {
    return score_type = sctype;
  }

  void add(ScoreArray& e);
  void add(const ScoreStats& e, const std::string& sent_idx);

  inline size_t NumberOfScores() const {
    return number_of_scores;
  }
  inline size_t size() const {
    return array_.size();
  }

  void save(const std::string &file, bool bin=false);
  void save(ofstream& outFile, bool bin=false);
  inline void save(bool bin=false) {
    save("/dev/stdout", bin);
  }

  void load(ifstream& inFile);
  void load(const std::string &file);

  bool check_consistency() const;
  void setIndex();

  inline int getIndex(const std::string& idx) const {
    name2idx::const_iterator i = arrayname2idx_.find(idx);
    if (i != arrayname2idx_.end())
      return i->second;
    else
      return -1;
  }
  inline std::string getIndex(size_t idx) const {
    idx2name::const_iterator i = idx2arrayname_.find(idx);
    if (i != idx2arrayname_.end())
      throw runtime_error("there is no entry at index " + idx);
    return i->second;
  }
};

#endif  // SCORE_DATA_H
