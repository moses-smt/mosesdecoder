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
#include "util/pool.hh"

class FeatureFunction;
class System;

class Scores {
	  friend std::ostream& operator<<(std::ostream &, const Scores &);
public:
  Scores(util::Pool &pool, size_t numScores);
  Scores(util::Pool &pool, size_t numScores, const Scores &origScores);
  virtual ~Scores();

  void CreateFromString(const std::string &str, const FeatureFunction &featureFunction, const System &system);

  void PlusEquals(const std::vector<SCORE> &scores, const FeatureFunction &featureFunction, const System &system);
  void PlusEquals(const Scores &scores, const System &system);
protected:
	SCORE *m_scores;
	SCORE m_total;
};

