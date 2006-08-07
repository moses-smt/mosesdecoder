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

#pragma once

#include <vector>
#include <algorithm>
#include "LanguageModelMultiFactor.h"
#include "LanguageModelSingleFactor.h"
#include "Phrase.h"

template<typename LMImpl>
class LanguageModel_Chunking : public LanguageModelMultiFactor
{	
protected:
	size_t m_realNGramOrder;
	LMImpl m_lmImpl;
public:
	LanguageModel_Chunking() {}
	
	void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder)
	{
		m_lmImpl.Load(fileName, factorCollection, factorType, weight, nGramOrder);
		m_realNGramOrder = 3; // fixed for now
	}
		
	void CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore) const
	{
		fullScore	= 0;
		ngramScore	= 0;
	
		size_t phraseSize = phrase.GetSize();
		std::vector<const FactorArray*> contextFactor;
		contextFactor.reserve(m_nGramOrder);
				
		// start of sentence
		for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
		{
			contextFactor.push_back(&phrase.GetFactorArray(currPos));
			fullScore += GetValue(contextFactor);
		}
		
		if (phraseSize >= m_nGramOrder)
		{
			contextFactor.push_back(&phrase.GetFactorArray(m_nGramOrder - 1));
			ngramScore = GetValue(contextFactor);
		}
		
		// main loop
		for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
		{ // used by hypo to speed up lm score calc
			for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
			{
				contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
			}
			contextFactor[m_nGramOrder - 1] = &phrase.GetFactorArray(currPos);
			float partScore = GetValue(contextFactor);			
			ngramScore += partScore;		
		}
		fullScore += ngramScore;	
	}
	
	float GetValue(const std::vector<const FactorArray*> &contextFactor) const
	{
		if (contextFactor.size() == 0)
		{
			return 0;
		}
		const Factor *factor = *contextFactor[contextFactor.size() - 1][m_factorType];
		if (factor->GetString().substr(0, 2) != "I-") // don't double-count chunking tags
		{
			return 0;
		}
	
		// create vector of just B-factors, in reverse order
		size_t currOrder = 0;
		std::vector<const Factor*> chunkContext;
		for (int currPos = (int)contextFactor.size() - 1 ; currPos >= 0 ; --currPos )
		{
			const Factor *factor = *contextFactor[currPos][m_factorType];
			if (factor->GetString().substr(0, 2) != "I-")
			{
				chunkContext.push_back(factor);
				if (++currOrder >= m_realNGramOrder)
					break;
			}
		}
	
		// create context factor the right way round
		std::reverse(chunkContext.begin(), chunkContext.end());
		// calc score on that phrase
		LanguageModelSingleFactor::State *finalState; // what shall we do with this ???
		return m_lmImpl.GetValue(chunkContext, finalState);
	}
	
};


