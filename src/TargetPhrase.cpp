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

#include <assert.h>
#include "TargetPhrase.h"
#include "PhraseDictionary.h"
#include "LanguageModel.h"

using namespace std;

TargetPhrase::TargetPhrase(FactorDirection direction, const PhraseDictionary *phraseDictionary)
:Phrase(direction)
#ifdef N_BEST
	,m_scoreComponent(phraseDictionary)
#endif
{
}

// used when creating translations of unknown words:
// TODO the two versions of SetScore have two problems:
//  1) they are badly named- computePhraseScores would probably be better
//  2) they duplicate way too much code between them
void TargetPhrase::SetScore(const LMList &languageModels, float weightWP)
{
	m_transScore = m_ngramScore = 0;	
	m_fullScore = weightWP;
	
	LMList::const_iterator lmIter;
	for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		FactorType lmFactorType = lm.GetFactorType();
		
		if (GetSize() > 0 && GetFactor(0, lmFactorType) != NULL)
		{ // contains factors used by this LM
			const float weightLM = lm.GetWeight();
	
			float fullScore, nGramScore;
	
#ifdef N_BEST
			(*lmIter)->CalcScore(*this, fullScore, nGramScore);
			size_t lmId = (*lmIter)->GetId();
			pair<size_t, float> store(lmId, nGramScore);
			m_ngramComponent.push_back(store);
#else
			(*lmIter)->CalcScore(*this, fullScore, nGramScore);
#endif
	
			m_fullScore   += fullScore * weightLM;
			m_ngramScore	+= nGramScore * weightLM;
		}
	}	
}

void TargetPhrase::SetScore(const vector<float> &scoreVector, const vector<float> &weightT,
	const LMList &languageModels, float weightWP)
{
	assert(weightT.size() == scoreVector.size());

	// calc average score if non-best
	m_transScore = 0;
	for (size_t i = 0 ; i < scoreVector.size() ; i++)
	{
		float score =  TransformScore(scoreVector[i]);
#ifdef N_BEST
		m_scoreComponent[i] = score;
#endif
		m_transScore += score * weightT[i];
	}

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
#ifdef N_BEST
			lm.CalcScore(*this, fullScore, nGramScore);
			size_t lmId = lm.GetId();
			pair<size_t, float> store(lmId, nGramScore);
			m_ngramComponent.push_back(store);
#else
			lm.CalcScore(*this, fullScore, nGramScore);
#endif
	
			// total LM score so far
			totalNgramScore  += nGramScore * weightLM;
			totalFullScore   += fullScore * weightLM;
		}
	}
  m_ngramScore = totalNgramScore;
	m_fullScore = m_transScore + totalFutureScore + totalFullScore
							- (this->GetSize() * weightWP);	 // word penalty

}

void TargetPhrase::SetWeights(const vector<float> &weightT)
{
#ifdef N_BEST
	m_transScore = 0;
	for (size_t i = 0 ; i < weightT.size() ; i++)
	{
		m_transScore += m_scoreComponent[i] * weightT[i];
	}
#endif
}

void TargetPhrase::ResetScore()
{
	m_transScore = m_fullScore = m_ngramScore = 0;
#ifdef N_BEST
	m_scoreComponent.Reset();
#endif
}

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  os << static_cast<const Phrase&>(tp) << " score=" << tp.m_transScore << ", cmpProb: " << tp.m_fullScore;
  return os;
}
