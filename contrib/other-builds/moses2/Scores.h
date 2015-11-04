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

class FeatureFunction;
class FeatureFunctions;
class System;

class Scores {
	  friend std::ostream& operator<<(std::ostream &, const Scores &);
public:
  Scores(MemPool &pool, size_t numScores);
  Scores(MemPool &pool, size_t numScores, const Scores &origScores);
  virtual ~Scores();

  SCORE GetTotalScore() const
  { return m_total; }

  void Reset(size_t numScores);

  void CreateFromString(const std::string &str,
		  const FeatureFunction &featureFunction,
		  const System &system,
		  bool transformScores);

  void PlusEquals(const System &system,
		  const FeatureFunction &featureFunction,
		  const SCORE &score);

  void PlusEquals(const System &system,
		  const FeatureFunction &featureFunction,
		  const std::vector<SCORE> &scores);

  void PlusEquals(const System &system,
		  const Scores &scores);

  void Assign(const System &system,
		  const FeatureFunction &featureFunction,
		  const SCORE &score);

  void Assign(const System &system,
		  const FeatureFunction &featureFunction,
		  const std::vector<SCORE> &scores);

  void Debug(std::ostream &out, const FeatureFunctions &ffs) const;

protected:
	SCORE *m_scores;
	SCORE m_total;
};

