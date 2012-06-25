#ifndef __PERMUTATIONSCORER_H__
#define __PERMUTATIONSCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits.h>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "Permutation.h"
#include "SentenceLevelScorer.h"

/**
  * Permutation
 **/
class PermutationScorer: public SentenceLevelScorer
{

public:
  PermutationScorer(const std::string &distanceMetric = "HAMMING",
                    const std::string &config = std::string());
  void setReferenceFiles(const std::vector<std::string>& referenceFiles);
  void prepareStats(size_t sid, const std::string& text, ScoreStats& entry);
  static const int SCORE_PRECISION;

  size_t NumberOfScores() const {
    //cerr << "PermutationScorer number of scores: 1" << endl;
    return 1;
  };
  bool useAlignment() const {
    //cout << "PermutationScorer::useAlignment returning true" << endl;
    return true;
  };

  void score(const candidates_t& candidates, const diffs_t& diffs,
             statscores_t& scores) const;

protected:
  statscore_t calculateScore(const std::vector<statscore_t>& scores);
  PermutationScorer(const PermutationScorer&);
  ~PermutationScorer() {};
  PermutationScorer& operator=(const PermutationScorer&);
  int getNumberWords (const std::string & line) const;

  distanceMetricReferenceChoice_t m_refChoiceStrategy;
  distanceMetric_t m_distanceMetric;

  // data extracted from reference files
  // A vector of permutations for each reference file
  std::vector< std::vector<Permutation> > m_referencePerms;
  std::vector<size_t> m_sourceLengths;
  std::vector<std::string> m_referenceAlignments;

private:
};
//TODO need to read in floats for scores - necessary for selecting mean reference strategy and for BLEU?


#endif //__PERMUTATIONSCORER_H

