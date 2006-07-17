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
#include <limits>
#include <iostream>
#include <fstream>

#include "NGramNode.h"

#include "LanguageModel.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"

using namespace std;

// static variable init
const LmId LanguageModel::UNKNOWN_LM_ID(0);

LanguageModel::LanguageModel() {}

void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore
														, list< std::pair<size_t, float> >	&ngramComponent) const
{
	fullScore	= 0;
	ngramScore	= 0;
	FactorType factorType = GetFactorType();

	size_t phraseSize = phrase.GetSize();
	vector<const Factor*> contextFactor;
	contextFactor.reserve(m_nGramOrder);
		
	// start of sentence
	for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
	{
		contextFactor.push_back(phrase.GetFactor(currPos, factorType));		
		fullScore += GetValue(contextFactor);
	}
	
	if (phraseSize >= m_nGramOrder)
	{
		contextFactor.push_back(phrase.GetFactor(m_nGramOrder - 1, factorType));
		ngramScore = GetValue(contextFactor);
	}
	
	// main loop
	for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
	{ // used by hypo to speed up lm score calc
		for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[m_nGramOrder - 1] = phrase.GetFactor(currPos, factorType);
		
		ngramScore += GetValue(contextFactor);		
	}
	fullScore += ngramScore;
	
#ifdef N_BEST
				size_t lmId = GetId();
				pair<size_t, float> store(lmId, ngramScore);
				ngramComponent.push_back(store);
#endif
}

