// $Id$

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
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "LMList.h"
#include "StaticData.h"

using namespace std;


TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase)
	: m_targetPhrase(targetPhrase),m_sourcePhrase(targetPhrase.GetSourcePhrase())
	,m_sourceWordsRange	(wordsRange)
{	// used by initial translation step

	// set score
	m_scoreBreakdown.PlusEquals(targetPhrase.GetScoreBreakdown());
}

TranslationOption::TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase)
	: m_targetPhrase(targetPhrase)
	,m_sourcePhrase(copy.m_sourcePhrase) // take source phrase pointer from initial translation option
	,m_sourceWordsRange	(copy.m_sourceWordsRange)
	,m_scoreBreakdown(copy.m_scoreBreakdown)
{ // used in creating the next translation step
	m_scoreBreakdown.PlusEquals(targetPhrase.GetScoreBreakdown());
}

TranslationOption::TranslationOption(const TranslationOption &copy
																		, const Phrase &inputPhrase
																		, const ScoreComponentCollection2 &additionalScore)
	: m_targetPhrase						(inputPhrase),m_sourcePhrase(copy.m_sourcePhrase)
, m_sourceWordsRange	(copy.m_sourceWordsRange)
, m_scoreBreakdown(copy.m_scoreBreakdown)
{ // used in creating the next generation step
	m_scoreBreakdown.PlusEquals(additionalScore);
}

TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase, int /*whatever*/)
: m_targetPhrase(targetPhrase)
,m_sourceWordsRange	(wordsRange)
,m_futureScore(0)
{ // used to create trans opt from unknown word
}

TranslationOption *TranslationOption::MergeTranslation(const TargetPhrase &targetPhrase) const
{
	if (m_targetPhrase.IsCompatible(targetPhrase))
	{
		TargetPhrase mergePhrase(targetPhrase);
		mergePhrase.MergeFactors(m_targetPhrase);
		TranslationOption *newTransOpt = new TranslationOption(*this, mergePhrase);
		return newTransOpt;
	}
	else
	{
		return NULL;
	}
}

TranslationOption *TranslationOption::MergeGeneration(const Phrase &inputPhrase
																	, const ScoreComponentCollection2& generationScore) const
{
	if (m_targetPhrase.IsCompatible(inputPhrase))
	{
		Phrase mergePhrase(inputPhrase);
		mergePhrase.MergeFactors(m_targetPhrase);
		TranslationOption *newTransOpt = new TranslationOption(*this, mergePhrase, generationScore);
		return newTransOpt;
	}
	else
		return NULL;
}

bool TranslationOption::Overlap(const Hypothesis &hypothesis) const
{
	const WordsBitmap &bitmap = hypothesis.GetWordsBitmap();
	return bitmap.Overlap(GetSourceWordsRange());
}

void TranslationOption::CalcScore(const LMList &allLM, float weightWordPenalty)
{
	// LM scores
	float m_ngramScore = 0;
	float retFullScore = 0;

	allLM.CalcScore(GetTargetPhrase(), retFullScore, m_ngramScore, &m_scoreBreakdown);
	// future score
	m_futureScore = retFullScore - m_ngramScore;

	size_t phraseSize = GetTargetPhrase().GetSize();
	m_futureScore += m_scoreBreakdown.InnerProduct(StaticData::Instance()->GetAllWeights()) - phraseSize * weightWordPenalty;
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

