// $Id: LanguageModel.cpp 3720 2010-11-18 10:27:30Z bhaddow $

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

#include <cassert>
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>

#include "FFState.h"
#include "LanguageModel.h"
#include "LanguageModelImplementation.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
LanguageModel::LanguageModel(ScoreIndexManager &scoreIndexManager, LanguageModelImplementation *implementation) :
	m_implementation(implementation)
{
	scoreIndexManager.AddScoreProducer(this);
#ifndef WITH_THREADS
	// ref counting handled by boost otherwise
	cerr << m_implementation;
	m_implementation->IncrementReferenceCount();
#endif
}

LanguageModel::LanguageModel(ScoreIndexManager &scoreIndexManager, LanguageModel *loadedLM) :
	m_implementation(loadedLM->m_implementation)
{
	scoreIndexManager.AddScoreProducer(this);
#ifndef WITH_THREADS
	// ref counting handled by boost otherwise
	m_implementation->IncrementReferenceCount();
#endif
}

LanguageModel::~LanguageModel()
{
#ifndef WITH_THREADS
	if(m_implementation->DecrementReferenceCount() == 0)
		delete m_implementation;
#endif
}

// don't inline virtual funcs...
size_t LanguageModel::GetNumScoreComponents() const
{
	return 1;
}

float LanguageModel::GetWeight() const {
	size_t lmIndex = StaticData::Instance().GetScoreIndexManager().
        	GetBeginIndex(GetScoreBookkeepingID());
	return StaticData::Instance().GetAllWeights()[lmIndex];
}

void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore) const
{
	
	fullScore	= 0;
	ngramScore	= 0;
	
	size_t phraseSize = phrase.GetSize();
  	if (!phraseSize) return;
	
	vector<const Word*> contextFactor;
	contextFactor.reserve(GetNGramOrder());
	std::auto_ptr<FFState> state(m_implementation->NewState((phrase.GetWord(0) == m_implementation->GetSentenceStartArray()) ?
		m_implementation->GetBeginSentenceState() : m_implementation->GetNullContextState()));
	size_t currPos = 0;
	while (currPos < phraseSize)
	{
		const Word &word = phrase.GetWord(currPos);
		
		if (word.IsNonTerminal())
		{ // do nothing. reset ngram. needed to score targbet phrases during pt loading in chart decoding
			if (!contextFactor.empty()) {
				// TODO: state operator= ?
				state.reset(m_implementation->NewState(m_implementation->GetNullContextState()));
				contextFactor.clear();
			}
		}
		else
		{
			ShiftOrPush(contextFactor, word);
			assert(contextFactor.size() <= GetNGramOrder());
			
			if (word == m_implementation->GetSentenceStartArray())
			{ // do nothing, don't include prob for <s> unigram
				assert(currPos == 0);
			}
			else
			{
				float partScore = m_implementation->GetValueGivenState(contextFactor, *state);
				fullScore += partScore;
				if (contextFactor.size() == GetNGramOrder())
					ngramScore += partScore;
			}
		}
		
		currPos++;
	}
}

void LanguageModel::CalcScoreChart(const Phrase &phrase
								, float &beginningBitsOnly
								, float &ngramScore) const
{ // TODO - get rid of this function
	
	beginningBitsOnly	= 0;
	ngramScore	= 0;
	
	size_t phraseSize = phrase.GetSize();
	if (!phraseSize) return;
	
	vector<const Word*> contextFactor;
	contextFactor.reserve(GetNGramOrder());
	std::auto_ptr<FFState> state(m_implementation->NewState((phrase.GetWord(0) == m_implementation->GetSentenceStartArray()) ?
		m_implementation->GetBeginSentenceState() : m_implementation->GetNullContextState()));
	size_t currPos = 0;
	while (currPos < phraseSize)
	{
		const Word &word = phrase.GetWord(currPos);
		assert(!word.IsNonTerminal());
		
		ShiftOrPush(contextFactor, word);
		assert(contextFactor.size() <= GetNGramOrder());
		
		if (word == m_implementation->GetSentenceStartArray())
		{ // do nothing, don't include prob for <s> unigram
			assert(currPos == 0);
		}
		else
		{
			float partScore = m_implementation->GetValueGivenState(contextFactor, *state);
			
			if (contextFactor.size() == GetNGramOrder())
				ngramScore += partScore;
			else
				beginningBitsOnly += partScore;
		}
		
		currPos++;
	}
}

void LanguageModel::ShiftOrPush(vector<const Word*> &contextFactor, const Word &word) const
{
	if (contextFactor.size() < GetNGramOrder())
	{
		contextFactor.push_back(&word);
	}
	else
	{ // shift
		for (size_t currNGramOrder = 0 ; currNGramOrder < GetNGramOrder() - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[GetNGramOrder() - 1] = &word;
	}
}
		
const FFState* LanguageModel::EmptyHypothesisState(const InputType &/*input*/) const {
	// This is actually correct.  The empty _hypothesis_ has <s> in it.  Phrases use m_emptyContextState.  
	return m_implementation->NewState(m_implementation->GetBeginSentenceState());
}

FFState* LanguageModel::Evaluate(
    const Hypothesis& hypo,
    const FFState* ps,
    ScoreComponentCollection* out) const {
	// In this function, we only compute the LM scores of n-grams that overlap a
	// phrase boundary. Phrase-internal scores are taken directly from the
	// translation option. In the unigram case, there is no overlap, so we don't
	// need to do anything.
	if(GetNGramOrder() <= 1)
		return NULL;

	clock_t t=0;
	IFVERBOSE(2) { t  = clock(); } // track time
	if (hypo.GetCurrTargetLength() == 0)
		return ps ? m_implementation->NewState(ps) : NULL;
	const size_t currEndPos = hypo.GetCurrTargetWordsRange().GetEndPos();
	const size_t startPos = hypo.GetCurrTargetWordsRange().GetStartPos();

	// 1st n-gram
	vector<const Word*> contextFactor(GetNGramOrder());
	size_t index = 0;
	for (int currPos = (int) startPos - (int) GetNGramOrder() + 1 ; currPos <= (int) startPos ; currPos++)
	{
		if (currPos >= 0)
			contextFactor[index++] = &hypo.GetWord(currPos);
		else
		{
			contextFactor[index++] = &m_implementation->GetSentenceStartArray();
		}
	}
  unsigned int statelen;
	FFState *res = m_implementation->NewState(ps);
	float lmScore = ps ? m_implementation->GetValueGivenState(contextFactor, *res, &statelen) : m_implementation->GetValueForgotState(contextFactor, *res, &statelen);

	// main loop
	size_t endPos = std::min(startPos + GetNGramOrder() - 2
			, currEndPos);
	for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
	{
		// shift all args down 1 place
		for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
			contextFactor[i] = contextFactor[i + 1];

		// add last factor
		contextFactor.back() = &hypo.GetWord(currPos);

		lmScore	+= m_implementation->GetValueGivenState(contextFactor, *res, &statelen);
	}

	// end of sentence
	if (hypo.IsSourceCompleted())
	{
		const size_t size = hypo.GetSize();
		contextFactor.back() = &m_implementation->GetSentenceEndArray();

		for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i ++)
		{
			int currPos = (int)(size - GetNGramOrder() + i + 1);
			if (currPos < 0)
				contextFactor[i] = &m_implementation->GetSentenceStartArray();
			else
				contextFactor[i] = &hypo.GetWord((size_t)currPos);
		}
		lmScore	+= m_implementation->GetValueForgotState(contextFactor, *res);
	} else {

		if (endPos < currEndPos){ 
			//need to get the LM state (otherwise the last LM state is fine)
			for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
				for (size_t i = 0 ; i < GetNGramOrder() - 1 ; i++)
					contextFactor[i] = contextFactor[i + 1];
				contextFactor.back() = &hypo.GetWord(currPos);
			}
		  m_implementation->GetState(contextFactor, *res);
		}
	}
	out->PlusEquals(this, lmScore);
	IFVERBOSE(2) { hypo.GetManager().GetSentenceStats().AddTimeCalcLM( clock()-t ); }
	return res;
}

}
