/*
 * TargetPhrase.h
 *
 *  Created on: 26 Apr 2016
 *      Author: hieu
 */

#pragma once
#include "Phrase.h"

namespace Moses2
{

class TargetPhrase: public Phrase<Word>
{
  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);

public:
  const PhraseTable &pt;
  mutable void **ffData;
  SCORE *scoreProperties;

  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system);

  Scores &GetScores()
  {
    return *m_scores;
  }

  const Scores &GetScores() const
  {
    return *m_scores;
  }

  SCORE GetFutureScore() const;

  void SetEstimatedScore(const SCORE &value)
  {
    m_estimatedScore = value;
  }

  SCORE *GetScoresProperty(int propertyInd) const;

protected:
  Scores *m_scores;
  SCORE m_estimatedScore;

};

//////////////////////////////////////////
struct CompareFutureScore
{
  bool operator()(const TargetPhrase *a, const TargetPhrase *b) const
  {
    return a->GetFutureScore() > b->GetFutureScore();
  }

  bool operator()(const TargetPhrase &a, const TargetPhrase &b) const
  {
    return a.GetFutureScore() > b.GetFutureScore();
  }
};


} /* namespace Moses2a */

