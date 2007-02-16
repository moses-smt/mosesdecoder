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
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LMList.h"
#include "StaticData.h"

using namespace std;

//TODO this should be a factory function!
TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase)
	: m_targetPhrase(targetPhrase),m_sourcePhrase(targetPhrase.GetSourcePhrase())
	,m_sourceWordsRange	(wordsRange)
{
	// set score
	m_scoreBreakdown.PlusEquals(targetPhrase.GetScoreBreakdown());
}

//TODO this should be a factory function!
TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase, int /*whatever*/)
: m_targetPhrase(targetPhrase)
,m_sourceWordsRange	(wordsRange)
,m_futureScore(0)
{
}

void TranslationOption::MergeNewFeatures(const Phrase& phrase, const ScoreComponentCollection& score, const std::vector<FactorType>& featuresToAdd)
{
	assert(phrase.GetSize() == m_targetPhrase.GetSize());
	if (featuresToAdd.size() == 1) {
		m_targetPhrase.MergeFactors(phrase, featuresToAdd[0]);
	} else if (featuresToAdd.empty()) {
		/* features already there, just update score */ 
  } else {
		m_targetPhrase.MergeFactors(phrase, featuresToAdd);
	}
	m_scoreBreakdown.PlusEquals(score);
}

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

void TranslationOption::CalcScore()
{
	// LM scores
	float m_ngramScore = 0;
	float retFullScore = 0;

	const LMList &allLM = StaticData::Instance().GetAllLM();

	allLM.CalcScore(GetTargetPhrase(), retFullScore, m_ngramScore, &m_scoreBreakdown);
	// future score
	m_futureScore = retFullScore - m_ngramScore;

	size_t phraseSize = GetTargetPhrase().GetSize();
	m_futureScore += m_scoreBreakdown.InnerProduct(StaticData::Instance().GetAllWeights()) - phraseSize * StaticData::Instance().GetWeightWordPenalty();
}

TO_STRING_BODY(TranslationOption);

// friend
ostream& operator<<(ostream& out, const TranslationOption& possibleTranslation)
{
	out << possibleTranslation.GetTargetPhrase() 
			<< "c=" << possibleTranslation.GetFutureScore()
			<< " [" << possibleTranslation.GetSourceWordsRange() << "]"
			<< possibleTranslation.GetScoreBreakdown();
	return out;
}
