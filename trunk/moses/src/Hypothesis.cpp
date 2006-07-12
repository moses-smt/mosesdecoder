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

#include <iostream>
#include <limits>
#include <assert.h>
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "Hypothesis.h"
#include "Util.h"
#include "Arc.h"
#include "SquareMatrix.h"

using namespace std;

Hypothesis::Hypothesis(const Phrase &phrase)
	: LatticeEdge(Output, NULL)
	, m_sourceCompleted(phrase.GetSize())
	, m_currSourceWordsRange(NOT_FOUND, NOT_FOUND)
	, m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
{	// used for initial seeding of trans process	
	// initialize scores
	ResetScore();	
}

Hypothesis::Hypothesis(const Hypothesis &copy)
	: LatticeEdge							(Output, copy.m_prevHypo)
	, m_sourceCompleted				(copy.m_sourceCompleted )
	, m_currSourceWordsRange	(copy.m_currSourceWordsRange)
	, m_currTargetWordsRange		(copy.m_currTargetWordsRange)
{
	m_phrase.AddWords( copy.m_phrase );

	// initialize scores
	SetScore(copy.GetScore());
#ifdef N_BEST
	m_lmScoreComponent 				= copy.GetLMScoreComponent();
	m_transScoreComponent			= copy.GetScoreComponent();
	m_generationScoreComponent	= copy.GetGenerationScoreComponent();
		
#endif
}

Hypothesis::Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt)
	: LatticeEdge							(Output, &prevHypo)
	, m_sourceCompleted				(prevHypo.m_sourceCompleted )
	, m_currSourceWordsRange	(prevHypo.m_currSourceWordsRange)
	, m_currTargetWordsRange		( prevHypo.m_currTargetWordsRange.GetEndPos() + 1
														 ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetPhrase().GetSize())
{
	const Phrase &possPhrase				= transOpt.GetPhrase();
	const WordsRange &wordsRange		= transOpt.GetWordsRange();
	m_currSourceWordsRange 					= wordsRange;
	m_sourceCompleted.SetValue(wordsRange.GetStartPos(), wordsRange.GetEndPos(), true);
	// add new words from poss trans
	//m_phrase.AddWords(prev.m_phrase);
	m_phrase.AddWords(possPhrase);

	// scores
	SetScore(prevHypo.GetScore());
	m_score[ScoreType::PhraseTrans]				+= transOpt.GetTranslationScore();
	m_score[ScoreType::FutureScoreEnum]		+= transOpt.GetFutureScore();
	m_score[ScoreType::LanguageModelScore]	+= transOpt.GetNgramScore();

#ifdef N_BEST
	// language model score (ngram)
	m_lmScoreComponent = prevHypo.GetLMScoreComponent();
	const list< pair<size_t, float> > &nGramComponent = transOpt.GetTrigramComponent();

	list< pair<size_t, float> >::const_iterator iter;
	for (iter = nGramComponent.begin() ; iter != nGramComponent.end() ; ++iter)
	{
		size_t lmId = (*iter).first;
		float score	= (*iter).second;
		m_lmScoreComponent[lmId] += score;
	}

	// translation score
	const ScoreComponentCollection &prevComponent= prevHypo.GetScoreComponent();
	m_transScoreComponent = prevComponent;
	
	// add components specific to poss trans
	const ScoreComponent &possComponent	= transOpt.GetScoreComponents();
	ScoreComponent &transComponent				= m_transScoreComponent.GetScoreComponent(possComponent.GetDictionary());
	
	for (size_t i = 0 ; i < NUM_PHRASE_SCORES ; i++)
	{
		transComponent[i] += possComponent[i];
	}

	// generation score
	m_generationScoreComponent = prevHypo.GetGenerationScoreComponent();
		
#endif
}

Hypothesis::~Hypothesis()
{
#ifdef N_BEST
	
	RemoveAllInColl< list<Arc*>::iterator >(m_arcList);
#endif
}

Hypothesis *Hypothesis::CreateNext(const TranslationOption &transOpt) const
{
	Hypothesis *clone	= new Hypothesis(*this, transOpt);
	return clone;
}

Hypothesis *Hypothesis::MergeNext(const TranslationOption &transOpt) const
{
	// check each word is compatible and merge 1-by-1
	const Phrase &possPhrase = transOpt.GetPhrase();
	if (! IsCompatible(possPhrase))
	{
		return NULL;
	}

	// ok, merge
	Hypothesis *clone				= new Hypothesis(*this);

	int currWord = 0;
	size_t len = GetSize();
	for (size_t currPos = len - m_currTargetWordsRange.GetWordsCount() ; currPos < len ; currPos++)
	{
		const FactorArray &sourceWord	= possPhrase.GetFactorArray(currWord);
		FactorArray &targetWord = clone->GetCurrFactorArray(currPos - m_currTargetWordsRange.GetStartPos());
		Word::Merge(targetWord, sourceWord);
		
		currWord++;
	}

#ifdef N_BEST
	const ScoreComponent &transOptComponent = transOpt.GetScoreComponents();
	clone->m_transScoreComponent.Remove(transOptComponent.GetDictionary());
	clone->m_transScoreComponent.Add(transOptComponent);
#endif

	return clone;

}

void Hypothesis::MergeFactors(vector< const Word* > mergeWords, const GenerationDictionary &generationDictionary, float generationScore, float weight)
{
	assert (mergeWords.size() == m_currTargetWordsRange.GetWordsCount());

	const size_t startPos = GetSize() - m_currTargetWordsRange.GetWordsCount();

	int mergeWordPos = 0;
	for (size_t currPos = startPos ; currPos < startPos + mergeWords.size() ; currPos++)
	{
		const Word &mergeWord = *mergeWords[mergeWordPos];
		FactorArray &origWord	= GetCurrFactorArray(currPos - m_currTargetWordsRange.GetStartPos());

		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *factor = mergeWord.GetFactor(factorType);
			if (factor != NULL)
			{
				origWord[factorType] = factor;
			}
		}

		mergeWordPos++;
	}

	// score
	m_score[ScoreType::Generation] += generationScore * weight;
#ifdef N_BEST
	m_generationScoreComponent[(size_t) &generationDictionary] += generationScore;
#endif
}

bool Hypothesis::IsCompatible(const Phrase &phrase) const
{
	// make sure factors don't contradict each other
	// similar to phrase comparison

	if (m_currTargetWordsRange.GetWordsCount() != phrase.GetSize())
	{
		return false;
	}
	size_t hypoSize = GetSize();

	size_t transOptPos = 0;
	for (size_t hypoPos = hypoSize - m_currTargetWordsRange.GetWordsCount() ; hypoPos < hypoSize ; hypoPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(hypoPos, factorType)
									,*compareFactor	= phrase.GetFactor(transOptPos, factorType);
			if (thisFactor != NULL && compareFactor != NULL && thisFactor != compareFactor)
				return false;
		}
		transOptPos++;
	}
	return true;
}

int Hypothesis::NGramCompare(const Hypothesis &compare, size_t nGramSize) const
{ // -1 = this < compare
	// +1 = this > compare
	// 0	= this ==compare

	size_t thisSize			= GetSize();
	size_t compareSize	= compare.GetSize();
	size_t minSize			= std::min(nGramSize, thisSize)
			, minCompareSize= std::min(nGramSize, compareSize);

	if ( minSize != minCompareSize )
	{ // quick decision
		return (minSize < minCompareSize) ? -1 : 1;
	}

	for (size_t currNGram = 1 ; currNGram <= minSize ; currNGram++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(thisSize - currNGram, factorType)
				,*compareFactor	= compare.GetFactor(compareSize - currNGram, factorType);
			// just use address of factor
			if (thisFactor < compareFactor)
				return -1;
			if (thisFactor > compareFactor)
				return 1;
		}		
	}
	// identical
	return 0;
}

void Hypothesis::CalcLMScore(const LMList &lmListInitial, const LMList	&lmListEnd)
{
	const size_t startPos	= m_currTargetWordsRange.GetStartPos();
	LMList::const_iterator iterLM;

	// for LM which are not in PossTran
	// must go through each trigram in current phrase
	for (iterLM = lmListEnd.begin() ; iterLM != lmListEnd.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType	= languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		// 1st n-gram
		vector<const Factor*> contextFactor(nGramOrder);
		size_t index = 0;
		for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
		{
			if (currPos >= 0)
				contextFactor[index++] = GetFactor(currPos, factorType);
			else			
				contextFactor[index++] = languageModel.GetSentenceStart();
		}		
		lmScore	= languageModel.GetValue(contextFactor);

		// main loop
		for (size_t currPos = startPos + 1 ; currPos <= m_currTargetWordsRange.GetEndPos() ; currPos++)
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			// add last factor
			contextFactor.back() = GetFactor(currPos, factorType);

			lmScore	+= languageModel.GetValue(contextFactor);
		}

		// end of sentence
		if (m_sourceCompleted.IsComplete())
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			contextFactor.back() = languageModel.GetSentenceEnd();
			lmScore	+= languageModel.GetValue(contextFactor);
		}
		m_score[ScoreType::LanguageModelScore] += lmScore * languageModel.GetWeight();
#ifdef N_BEST
		size_t lmId = languageModel.GetId();
		m_lmScoreComponent[lmId] += lmScore;
#endif
	}

	// for LM which are in possTran
	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = lmListInitial.begin() ; iterLM != lmListInitial.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType = languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		// 1st n-gram
		vector<const Factor*> contextFactor(nGramOrder);
		size_t index = 0;
		for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
		{
			if (currPos >= 0)
				contextFactor[index++] = GetFactor(currPos, factorType);
			else			
				contextFactor[index++] = languageModel.GetSentenceStart();
		}		
		lmScore	= languageModel.GetValue(contextFactor);

		// main loop
		size_t endPos = std::min(startPos + nGramOrder - 2
														, m_currTargetWordsRange.GetEndPos());
		for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			// add last factor
			contextFactor.back() = GetFactor(currPos, factorType);

			lmScore	+= languageModel.GetValue(contextFactor);
		}

		// end of sentence
		if (m_sourceCompleted.IsComplete())
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			contextFactor.back() = languageModel.GetSentenceEnd();
			lmScore	+= languageModel.GetValue(contextFactor);
		}
		m_score[ScoreType::LanguageModelScore] += lmScore * languageModel.GetWeight();
#ifdef N_BEST
		size_t lmId = languageModel.GetId();
		m_lmScoreComponent[lmId] += lmScore;
#endif
	}

}

void Hypothesis::CalcScore(const LMList		&lmListInitial
													, const LMList	&lmListEnd
													, float weightDistortion
													, float weightWordPenalty
													, const SquareMatrix &futureScore)
{
	// DISTORTION COST
	const WordsRange &prevRange = m_prevHypo->GetCurrSourceWordsRange()
								, &currRange	= GetCurrSourceWordsRange();
				
	if (prevRange.GetWordsCount() == 0)
	{ // 1st hypothesis with translated phrase. NOT the seed hypo.
		m_score[ScoreType::Distortion]	=  - (float) currRange.GetStartPos();
	}
	else
	{ // add distortion score of current translated phrase to
		// distortions scores of all previous partial translations
		m_score[ScoreType::Distortion]	-=  (float) currRange.CalcDistortion(prevRange) ;
	}
	
	// LANGUAGE MODEL COST
	CalcLMScore(lmListInitial, lmListEnd);

	// WORD PENALTY
	m_score[ScoreType::WordPenalty] = - (float) GetSize();

	// FUTURE COST
	CalcFutureScore(futureScore);

	// TOTAL COST
	m_score[ScoreType::Total] = m_score[ScoreType::PhraseTrans]
								+ m_score[ScoreType::Generation]			
								+ m_score[ScoreType::LanguageModelScore]
								+ m_score[ScoreType::Distortion]					* weightDistortion
								+ m_score[ScoreType::WordPenalty]				* weightWordPenalty
								+ m_score[ScoreType::FutureScoreEnum];
}

void Hypothesis::CalcFutureScore(const SquareMatrix &futureScore)
{
	const size_t maxSize= numeric_limits<size_t>::max();
	size_t	start				= maxSize;
	m_score[ScoreType::FutureScoreEnum]	= 0;
	for(size_t currPos = 0 ; currPos < m_sourceCompleted.GetSize() ; currPos++) 
	{
		if(m_sourceCompleted.GetValue(currPos) == 0 && start == maxSize)
		{
			start = currPos;
		}
		if(m_sourceCompleted.GetValue(currPos) == 1 && start != maxSize) 
		{
			m_score[ScoreType::FutureScoreEnum] += futureScore.GetScore(start, currPos - 1);
			start = maxSize;
		}
	}
	if (start != maxSize)
	{
		m_score[ScoreType::FutureScoreEnum] += futureScore.GetScore(start, m_sourceCompleted.GetSize() - 1);
	}
}

// friend
ostream& operator<<(ostream& out, const Hypothesis& hypothesis)
{	
	hypothesis.ToStream(out);
	// words bitmap
	out << "[" << hypothesis.m_sourceCompleted << "] ";
	
	// scores
	out << " [" << hypothesis.GetScore( static_cast<ScoreType::ScoreType>(0));
	for (size_t i = 1 ; i < NUM_SCORES ; i++)
	{
		out << "," << hypothesis.GetScore( static_cast<ScoreType::ScoreType>(i));
	}
	out << "]";
#ifdef N_BEST
	out << " " << hypothesis.GetScoreComponent();
	out << " " << hypothesis.GetGenerationScoreComponent();
#endif
	return out;
}

