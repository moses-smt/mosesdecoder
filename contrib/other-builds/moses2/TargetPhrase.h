/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "Phrase.h"
#include "util/pool.hh"

class Scores;
class Manager;
class StaticData;

class TargetPhrase : public Phrase
{
public:
  static TargetPhrase *CreateFromString(util::Pool *pool, StaticData &staticData, const std::string &str);
  TargetPhrase(util::Pool *pool, StaticData &staticData, size_t size);
  virtual ~TargetPhrase();

  Scores &GetScores()
  { return *m_scores; }

protected:
	Scores *m_scores;
};

