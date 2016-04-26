/*
 * PhraseImpl.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/functional/hash.hpp>
#include "Phrase.h"
#include "Word.h"
#include "MemPool.h"
#include "Scores.h"
#include "System.h"

using namespace std;

namespace Moses2
{


////////////////////////////////////////////////////////////////////////
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

} // namespace

