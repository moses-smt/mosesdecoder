/*
 * TargetPhrase.cpp
 *
 *  Created on: 26 Apr 2016
 *      Author: hieu
 */

#include "TargetPhrase.h"
#include "System.h"
#include "Scores.h"

namespace Moses2
{

TargetPhrase::TargetPhrase(MemPool &pool, const PhraseTable &pt,
    const System &system) :
    pt(pt), scoreProperties(NULL)
{
  m_scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores());
}

SCORE TargetPhrase::GetFutureScore() const
{
  return m_scores->GetTotalScore() + m_estimatedScore;
}

SCORE *TargetPhrase::GetScoresProperty(int propertyInd) const
{
  return scoreProperties ? scoreProperties + propertyInd : NULL;
}

std::ostream& operator<<(std::ostream &out, const TargetPhrase &obj)
{
  out << (const Phrase<Word> &) obj << " SCORES:" << obj.GetScores();
  return out;
}

} /* namespace Moses2a */
