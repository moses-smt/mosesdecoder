// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "TranslationOption.h"
#include "WordsBitmap.h"
#include "GenerationDictionary.h"
#include "LMList.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "InputType.h"
#include "DummyScoreProducers.h"

using namespace std;

namespace Moses
{

//TODO this should be a factory function!
TranslationOption::TranslationOption(const WordsRange &wordsRange
                                     , const TargetPhrase &targetPhrase)
  : m_targetPhrase(targetPhrase)
  , m_sourceWordsRange(wordsRange)
  , m_futureScore(targetPhrase.GetFutureScore())
{
}

TranslationOption::TranslationOption(const TranslationOption &copy, const WordsRange &sourceWordsRange)
: m_targetPhrase(copy.m_targetPhrase)
//, m_sourcePhrase(new Phrase(*copy.m_sourcePhrase)) // TODO use when confusion network trans opt for confusion net properly implemented
, m_sourceWordsRange(sourceWordsRange)
, m_futureScore(copy.m_futureScore)
, m_lexReorderingScores(copy.m_lexReorderingScores)
{}

bool TranslationOption::IsCompatible(const Phrase& phrase, const std::vector<FactorType>& featuresToCheck) const
{
  if (featuresToCheck.size() == 1) {
    return m_targetPhrase.IsCompatible(phrase, featuresToCheck[0]);
  } else if (featuresToCheck.empty()) {
    return true;
    /* features already there, just update score */
  } else {
    return m_targetPhrase.IsCompatible(phrase, featuresToCheck);
  }
}

bool TranslationOption::Overlap(const Hypothesis &hypothesis) const
{
  const WordsBitmap &bitmap = hypothesis.GetWordsBitmap();
  return bitmap.Overlap(GetSourceWordsRange());
}

TO_STRING_BODY(TranslationOption);

// friend
ostream& operator<<(ostream& out, const TranslationOption& possibleTranslation)
{
  out << possibleTranslation.GetTargetPhrase()
      << " c=" << possibleTranslation.GetFutureScore()
      << " [" << possibleTranslation.GetSourceWordsRange() << "]"
      << possibleTranslation.GetScoreBreakdown();
  return out;
}

void TranslationOption::CacheLexReorderingScores(const FeatureFunction &producer, const Scores &score)
{
  m_lexReorderingScores[&producer] = score;
}

}


