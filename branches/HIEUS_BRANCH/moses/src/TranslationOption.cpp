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
#include "GenerationDictionary.h"
#include "LMList.h"

using namespace std;


TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase)
: m_phrase(targetPhrase)
,m_sourceWordsRange	(wordsRange)
{	// used by initial translation step

	// set score
	m_scoreGen		= 0;
	m_scoreTrans	= targetPhrase.GetTranslationScore();
#ifdef N_BEST
	m_transScoreComponent.Add(targetPhrase.GetScoreComponents());
#endif
}

TranslationOption::TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase)
: m_phrase(targetPhrase)
,m_sourceWordsRange	(copy.m_sourceWordsRange)
#ifdef N_BEST
,m_transScoreComponent(copy.m_transScoreComponent)
,m_generationScoreComponent(copy.m_generationScoreComponent)
#endif
{ // used in creating the next translation step
	m_scoreGen		= copy.GetGenerationScore();
	m_scoreTrans	= copy.GetTranslationScore() + targetPhrase.GetTranslationScore();
	
	#ifdef N_BEST
		m_transScoreComponent.Add(targetPhrase.GetScoreComponents());
	#endif
}

TranslationOption::TranslationOption(const TranslationOption &copy
																		, const Phrase &inputPhrase
																		, const GenerationDictionary *generationDictionary
																		, float generationScore
																		, float weight)
: m_phrase						(inputPhrase)
, m_sourceWordsRange	(copy.m_sourceWordsRange)
#ifdef N_BEST
,m_transScoreComponent(copy.m_transScoreComponent)
,m_generationScoreComponent(copy.m_generationScoreComponent)
#endif
{ // used in creating the next generation step

	m_scoreTrans	= copy.GetTranslationScore();
	m_scoreGen	= copy.GetGenerationScore() + generationScore * weight;

	#ifdef N_BEST
		m_generationScoreComponent[(size_t)generationDictionary] = generationScore;
	#endif
}

TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase
																		 , list<const PhraseDictionary*>			&allPhraseDictionary
																		 , list<const GenerationDictionary*>	&allGenerationDictionary)
: m_phrase(targetPhrase)
,m_sourceWordsRange	(wordsRange)
,m_scoreTrans(0)
,m_scoreGen(0)
,m_futureScore(0)
,m_ngramScore(0)
{ // used to create trans opt from unknown word

	#ifdef N_BEST
		// create score components
		list<const PhraseDictionary*>::iterator iterPhraseDict;
		for (iterPhraseDict = allPhraseDictionary.begin() ; iterPhraseDict != allPhraseDictionary.end() ; ++iterPhraseDict)
		{
			const PhraseDictionary *dict = *iterPhraseDict;
			m_transScoreComponent.Add(dict);
		}

		list<const GenerationDictionary*>::iterator iterGenDict;
		for (iterGenDict = allGenerationDictionary.begin() ; iterGenDict != allGenerationDictionary.end() ; ++iterGenDict)
		{
			const GenerationDictionary *dict = *iterGenDict;
			m_generationScoreComponent.Add((size_t)dict);
		}
	#endif
}

TranslationOption *TranslationOption::MergeTranslation(const TargetPhrase &targetPhrase) const
{
	if (m_phrase.IsCompatible(targetPhrase))
	{
		TargetPhrase mergePhrase(targetPhrase);
		mergePhrase.MergeFactors(m_phrase);
		TranslationOption *newTransOpt = new TranslationOption(*this, mergePhrase);
		return newTransOpt;
	}
	else
	{
		return NULL;
	}
}

TranslationOption *TranslationOption::MergeGeneration(const Phrase &inputPhrase
																	, const GenerationDictionary *generationDictionary
																	, float generationScore
																	, float weight) const
{
	if (m_phrase.IsCompatible(inputPhrase))
	{
		Phrase mergePhrase(inputPhrase);
		mergePhrase.MergeFactors(m_phrase);
		TranslationOption *newTransOpt = new TranslationOption(*this, mergePhrase, generationDictionary, generationScore, weight);
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
	m_ngramScore = 0;
	float retFullScore = 0;

	LMList::const_iterator iter;
	for (iter = allLM.begin() ; iter != allLM.end() ; ++iter)
	{
		const LanguageModel &lm = **iter;
		m_ngramComponent.Add(lm.GetId());
	}
	#ifdef N_BEST
		allLM.CalcScore(GetTargetPhrase(), retFullScore, m_ngramScore, &m_ngramComponent);
	#else
		allLM.CalcScore(GetTargetPhrase(), retFullScore, m_ngramScore, NULL);
	#endif
	// future score
	m_futureScore = retFullScore;

	size_t phraseSize = GetTargetPhrase().GetSize();
	m_futureScore += m_scoreTrans - phraseSize * weightWordPenalty;
}

// friend
ostream& operator<<(ostream& out, const TranslationOption& possibleTranslation)
{
	out << possibleTranslation.GetTargetPhrase() 
			<< ", pC=" << possibleTranslation.GetTranslationScore()
			<< ", c=" << possibleTranslation.GetFutureScore()
			<< " [" << possibleTranslation.GetSourceWordsRange() << "]";
	return out;
}

