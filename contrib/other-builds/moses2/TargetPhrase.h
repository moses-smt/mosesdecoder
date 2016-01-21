/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "Phrase.h"
#include "MemPool.h"

namespace Moses2
{

class Scores;
class Manager;
class System;
class PhraseTable;

class TargetPhrase : public PhraseImpl
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:
  mutable void **ffData;
  SCORE *scoreProperties;
  const PhraseTable &pt;

  static TargetPhrase *CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str);
  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size);
  //TargetPhrase(MemPool &pool, const System &system, const TargetPhrase &copy);

  virtual ~TargetPhrase();

  Scores &GetScores()
  { return *m_scores; }

  const Scores &GetScores() const
  { return *m_scores; }

  SCORE GetFutureScore() const;

  void SetEstimatedScore(const SCORE &value)
  { m_estimatedScore = value; }

  SCORE *GetScoresProperty(int propertyInd) const;
protected:
	Scores *m_scores;
	SCORE m_estimatedScore;
};

//////////////////////////////////////////
struct CompareFutureScore {
  bool operator() (const TargetPhrase *a, const TargetPhrase *b) const
  {
	  return a->GetFutureScore() > b->GetFutureScore();
  }

  bool operator() (const TargetPhrase &a, const TargetPhrase &b) const
  {
	  return a.GetFutureScore() > b.GetFutureScore();
  }
};

}

