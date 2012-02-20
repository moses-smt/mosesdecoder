/*
 *  Data.h
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef MERT_DATA_H_
#define MERT_DATA_H_

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include<boost/shared_ptr.hpp>

#include "Util.h"
#include "FeatureData.h"
#include "ScoreData.h"

class Scorer;

typedef boost::shared_ptr<ScoreData> ScoreDataHandle;
typedef boost::shared_ptr<FeatureData> FeatureDataHandle;

class Data
{
private:
  Scorer* theScorer;
  std::string score_type;
  size_t number_of_scores;
  bool _sparse_flag;

protected:
  ScoreDataHandle scoredata;
  FeatureDataHandle featdata;

public:
  explicit Data(Scorer& sc);
  Data();

  //Note that there is no copy constructor implemented, so only the 
  //compiler synthesised shallow copy is available

  inline void clear() {
    scoredata->clear();
    featdata->clear();
  }

  ScoreDataHandle getScoreData() {
    return scoredata;
  }

  FeatureDataHandle getFeatureData() {
    return featdata;
  }

  Scorer* getScorer() {
    return theScorer;
  }

  inline size_t NumberOfFeatures() const {
    return featdata->NumberOfFeatures();
  }
  inline void NumberOfFeatures(size_t v) {
    featdata->NumberOfFeatures(v);
  }
  inline std::string Features() const {
    return featdata->Features();
  }
  inline void Features(const std::string &f) {
    featdata->Features(f);
  }

  inline bool hasSparseFeatures() const { return _sparse_flag; }
  void mergeSparseFeatures();

  void loadnbest(const std::string &file);
  
  void load(const std::string &featfile,const std::string &scorefile) {
    featdata->load(featfile);
    scoredata->load(scorefile);
    if (featdata->hasSparseFeatures())
      _sparse_flag = true;
  }

  //ADDED BY TS
  void remove_duplicates();
  //END_ADDED

  void save(const std::string &featfile,const std::string &scorefile, bool bin=false) {

    if (bin) cerr << "Binary write mode is selected" << endl;
    else cerr << "Binary write mode is NOT selected" << endl;

    featdata->save(featfile, bin);
    scoredata->save(scorefile, bin);
  }

  inline bool existsFeatureNames() const {
    return featdata->existsFeatureNames();
  }

  inline std::string getFeatureName(size_t idx) const {
    return featdata->getFeatureName(idx);
  }

  inline size_t getFeatureIndex(const std::string& name) const {
    return featdata->getFeatureIndex(name);
  }

  /**
   * Create shard_count shards. If shard_size == 0, then the shards are non-overlapping
   * and exhaust the data. If 0 < shard_size <= 1, then shards are chosen by sampling
   * the data (with replacement) and shard_size is interpreted as the proportion
   * of the total size.
   */
  void createShards(size_t shard_count, float shard_size, const std::string& scorerconfig,
                    std::vector<Data>& shards);
};

#endif  // MERT_DATA_H_
