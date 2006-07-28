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
#include "TargetPhrase.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "LanguageModel.h"
#include "StaticData.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"

using namespace std;

TargetPhrase::TargetPhrase(FactorDirection direction, const PhraseDictionaryBase *phraseDictionary)
:Phrase(direction),m_transScore(0.0), m_ngramScore(0.0), m_fullScore(0.0)
#ifdef N_BEST
	,m_inputScore(0.0)
	,m_sp(phraseDictionary)
#endif
{
}

TargetPhrase::TargetPhrase(FactorDirection direction)
	:Phrase(direction),m_transScore(0.0), m_ngramScore(0.0), m_fullScore(0.0)
#ifdef N_BEST
	,m_inputScore(0.0)
	,m_sp(0)
#endif
{
}

void TargetPhrase::SetScore(float weightWP)
{ // used when creating translations of unknown words:
	m_transScore = m_ngramScore = 0;	
	m_fullScore = - weightWP;	
}

void TargetPhrase::SetScore(const vector<float> &scoreVector, const vector<float> &weightT,
														const LMList &languageModels, float weightWP,float inputScore, float weightInput)
{
	assert(weightT.size() == scoreVector.size());
	// calc average score if non-best
	m_transScore = 0;
	for (size_t i = 0 ; i < scoreVector.size() ; i++)
	{
		float score =  TransformScore(scoreVector[i]);
		m_transScore += score * weightT[i];
	}

  #ifdef N_BEST
	m_inputScore=inputScore;
	vector<float> transScores(scoreVector.size());
	std::transform(scoreVector.begin(),scoreVector.end(),transScores.begin(),TransformScore);
	m_scoreBreakdown.PlusEquals(m_sp, transScores);
  #endif

  // Replicated from TranslationOptions.cpp
	float totalFutureScore = 0;
	float totalNgramScore  = 0;
	float totalFullScore   = 0;

	LMList::const_iterator lmIter;
	for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		FactorType lmFactorType = lm.GetFactorType();
		
		if (GetSize() > 0 && GetFactor(0, lmFactorType) != NULL)
		{ // contains factors used by this LM
			const float weightLM = lm.GetWeight();
			float fullScore, nGramScore;

			lm.CalcScore(*this, fullScore, nGramScore);
			#ifdef N_BEST
				m_scoreBreakdown.Assign(&lm, nGramScore);
			#endif

			// total LM score so far
			totalNgramScore  += nGramScore * weightLM;
			totalFullScore   += fullScore * weightLM;
			
		}
	}
  m_ngramScore = totalNgramScore;
	m_transScore += inputScore * weightInput;

	m_fullScore = m_transScore + totalFutureScore + totalFullScore
							- (this->GetSize() * weightWP);	 // word penalty
}

void TargetPhrase::SetWeights(const vector<float> &weightT)
{
#ifdef N_BEST
	m_transScore = m_scoreBreakdown.PartialInnerProduct(m_sp, weightT);
#endif
}

void TargetPhrase::ResetScore()
{
	m_transScore = m_fullScore = m_ngramScore = 0;
#ifdef N_BEST
	m_scoreBreakdown.ZeroAll();
#endif
}

TargetPhrase *TargetPhrase::MergeNext(const TargetPhrase &inputPhrase) const
{
	if (! IsCompatible(inputPhrase))
	{
		return NULL;
	}

	// ok, merge
	TargetPhrase *clone				= new TargetPhrase(*this);

	int currWord = 0;
	const size_t len = GetSize();
	for (size_t currPos = 0 ; currPos < len ; currPos++)
	{
		const FactorArray &inputWord	= inputPhrase.GetFactorArray(currPos);
		FactorArray &cloneWord = clone->GetFactorArray(currPos);
		Word::Merge(cloneWord, inputWord);
		
		currWord++;
	}

	return clone;
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  os << static_cast<const Phrase&>(tp) << ", pC=" << tp.m_transScore << ", c=" << tp.m_fullScore;
  return os;
}
