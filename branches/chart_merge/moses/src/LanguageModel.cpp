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

#include <cassert>
#include <limits>
#include <iostream>
#include <sstream>

#include "FFState.h"
#include "LanguageModel.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
LanguageModel::LanguageModel(bool registerScore, ScoreIndexManager &scoreIndexManager) 
{
	if (registerScore)
		scoreIndexManager.AddScoreProducer(this);
}
LanguageModel::~LanguageModel() {}

// don't inline virtual funcs...
size_t LanguageModel::GetNumScoreComponents() const
{
	return 1;
}

void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore) const
{
	fullScore	= 0;
	ngramScore	= 0;

	size_t phraseSize = phrase.GetSize();
	vector<const Word*> contextFactor;
	contextFactor.reserve(m_nGramOrder);

	// start of sentence
	for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
	{
		contextFactor.push_back(&phrase.GetWord(currPos));		
		fullScore += GetValue(contextFactor);
	}
	
	if (phraseSize >= m_nGramOrder)
	{
		contextFactor.push_back(&phrase.GetWord(m_nGramOrder - 1));
		ngramScore = GetValue(contextFactor);
	}
	
	// main loop
	for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
	{ // used by hypo to speed up lm score calc
		for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[m_nGramOrder - 1] = &phrase.GetWord(currPos);
		float partScore = GetValue(contextFactor);		
		ngramScore += partScore;		
	}
	fullScore += ngramScore;	
}

LanguageModel::State LanguageModel::GetState(const std::vector<const Word*> &contextFactor, unsigned int* len) const
{
  State state;
	unsigned int dummy;
  if (!len) len = &dummy;
  GetValue(contextFactor,&state,len);
  return state;
}

struct LMState : public FFState {
	const void* lmstate;
	LMState(const void* lms) { lmstate = lms; }
	virtual int Compare(const FFState& o) const {
		const LMState& other = static_cast<const LMState&>(o);
		if (other.lmstate > lmstate) return 1;
		else if (other.lmstate < lmstate) return -1;
		return 0;
	}
};

const FFState* LanguageModel::EmptyHypothesisState(const InputType &input) const {
	return new LMState(NULL);
}

FFState* LanguageModel::Evaluate(
    const Hypothesis& hypo,
    const FFState* ps,
    ScoreComponentCollection* out) const {
	// In this function, we only compute the LM scores of n-grams that overlap a
	// phrase boundary. Phrase-internal scores are taken directly from the
	// translation option. In the unigram case, there is no overlap, so we don't
	// need to do anything.
	if(m_nGramOrder <= 1)
		return NULL;

	clock_t t=0;
	IFVERBOSE(2) { t  = clock(); } // track time
	const void* prevlm = ps ? (static_cast<const LMState *>(ps)->lmstate) : NULL;
	LMState* res = new LMState(prevlm);
	if (hypo.GetCurrTargetLength() == 0)
		return res;
	const size_t currEndPos = hypo.GetCurrTargetWordsRange().GetEndPos();
	const size_t startPos = hypo.GetCurrTargetWordsRange().GetStartPos();

	// 1st n-gram
	vector<const Word*> contextFactor(m_nGramOrder);
	size_t index = 0;
	for (int currPos = (int) startPos - (int) m_nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
	{
		if (currPos >= 0)
			contextFactor[index++] = &hypo.GetWord(currPos);
		else			
			contextFactor[index++] = &GetSentenceStartArray();
	}
	float lmScore	= GetValue(contextFactor);
	//cout<<"context factor: "<<GetValue(contextFactor)<<endl;

	// main loop
	size_t endPos = std::min(startPos + m_nGramOrder - 2
			, currEndPos);
	for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
	{
		// shift all args down 1 place
		for (size_t i = 0 ; i < m_nGramOrder - 1 ; i++)
			contextFactor[i] = contextFactor[i + 1];

		// add last factor
		contextFactor.back() = &hypo.GetWord(currPos);

		lmScore	+= GetValue(contextFactor);
	}

	// end of sentence
	if (hypo.IsSourceCompleted())
	{
		const size_t size = hypo.GetSize();
		contextFactor.back() = &GetSentenceEndArray();

		for (size_t i = 0 ; i < m_nGramOrder - 1 ; i ++)
		{
			int currPos = (int)(size - m_nGramOrder + i + 1);
			if (currPos < 0)
				contextFactor[i] = &GetSentenceStartArray();
			else
				contextFactor[i] = &hypo.GetWord((size_t)currPos);
		}
		lmScore	+= GetValue(contextFactor, &res->lmstate);
	} else {
		for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
			for (size_t i = 0 ; i < m_nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];
			contextFactor.back() = &hypo.GetWord(currPos);
		}
		res->lmstate = GetState(contextFactor);
	}
	out->PlusEquals(this, lmScore);
  IFVERBOSE(2) { hypo.GetManager().GetSentenceStats().AddTimeCalcLM( clock()-t ); }
	return res;
}

}
