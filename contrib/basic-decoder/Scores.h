#pragma once

#include <vector>
#include <string>
#include "TypeDef.h"

class FeatureFunction;

class Scores
{
public:
  Scores();
  Scores(const Scores &copy);
  virtual ~Scores();
  void CreateFromString(const FeatureFunction &ff, const std::string &line, bool logScores);

  SCORE GetWeightedScore() const {
    return m_weightedScore;
  }

  void Add(const Scores &other);
  void Add(const FeatureFunction &ff, SCORE score);
  void Add(const FeatureFunction &ff, const std::vector<SCORE> &scores);

  std::string Debug() const;

protected:
#ifdef SCORE_BREAKDOWN
  std::vector<SCORE> m_scores; // maybe it doesn't need this
#endif
  SCORE m_weightedScore;
};
