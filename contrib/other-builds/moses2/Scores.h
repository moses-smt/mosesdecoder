/*
 * Scores.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <iostream>
#include <string>
#include "TypeDef.h"
#include "MemPool.h"
#include "Vector.h"

namespace Moses2
{

class FeatureFunction;
class FeatureFunctions;
class System;

class Scores
{
public:
  Scores(const System &system, MemPool &pool, size_t numScores);
  Scores(const System &system, MemPool &pool, size_t numScores,
      const Scores &origScores);

  virtual ~Scores();

  SCORE GetTotalScore() const
  {  return m_total; }

  const SCORE *GetScores(const FeatureFunction &featureFunction) const;
  const SCORE GetScore(const FeatureFunction &featureFunction, size_t offset) const;

  void Reset(const System &system);

  void CreateFromString(const std::string &str,
      const FeatureFunction &featureFunction, const System &system,
      bool transformScores);

  void PlusEquals(const System &system, const FeatureFunction &featureFunction,
      const SCORE &score);

  void PlusEquals(const System &system, const FeatureFunction &featureFunction,
      const SCORE &score, size_t offset);

  void PlusEquals(const System &system, const FeatureFunction &featureFunction,
      const std::vector<SCORE> &scores);

  void PlusEquals(const System &system, const FeatureFunction &featureFunction,
      const Vector<SCORE> &scores);

  void PlusEquals(const System &system, const FeatureFunction &featureFunction,
      SCORE scores[]);

  void PlusEquals(const System &system, const Scores &scores);

  void MinusEquals(const System &system, const Scores &scores);

  void Assign(const System &system, const FeatureFunction &featureFunction,
      const SCORE &score);

  void Assign(const System &system, const FeatureFunction &featureFunction,
      const std::vector<SCORE> &scores);

  std::ostream &Debug(std::ostream &out, const System &system) const;

  // static functions to work out estimated scores
  static SCORE CalcWeightedScore(const System &system,
      const FeatureFunction &featureFunction, SCORE scores[]);

  static SCORE CalcWeightedScore(const System &system,
      const FeatureFunction &featureFunction, SCORE score);

protected:
  SCORE *m_scores;
  SCORE m_total;
};

}

