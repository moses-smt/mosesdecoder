/*
 * Scores.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <string>
#include "TypeDef.h"
#include "util/pool.hh"

class FeatureFunction;
class StaticData;

class Scores {
public:
	Scores(util::Pool &pool, size_t numScores);
	virtual ~Scores();

	  void CreateFromString(const std::string &str, const FeatureFunction &featureFunction, const StaticData &staticData);
protected:
	SCORE *m_scores;
};

