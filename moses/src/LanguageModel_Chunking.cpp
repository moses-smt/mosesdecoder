
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

#include "LanguageModel_Chunking.h"
#include "Phrase.h"
#include <vector>

using namespace std;

LanguageModel_Chunking::LanguageModel_Chunking()
{
}
LanguageModel_Chunking::~LanguageModel_Chunking()
{
}

void LanguageModel_Chunking::Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder
					, size_t maxNGramOrder)
{
	MyBase::Load(fileName, factorCollection, factorType, weight, nGramOrder);
	m_maxNGramOrder = maxNGramOrder;
}

void LanguageModel_Chunking::CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore) const
{
	size_t currOrder = 0;
	vector<const Factor*> factors;
	for (int currPos = (int)phrase.GetSize() - 1 ; currPos >= 0 ; currPos++ )
	{
		const Factor *factor = phrase.GetFactor(currPos, m_factorType);
		if (factor->GetString().substr(0, 2) == "I-")
		{
			factors.push_back(factor);
			if (++currOrder >= m_nGramOrder)
				break;
		}
	}

	// create a phrase with only those factors
	Phrase phraseCalc(phrase.GetDirection());
	for (int currPos = (int)factors.size() - 1 ; currPos >= 0 ; currPos++ )
	{
		FactorArray &factorArray = phraseCalc.AddWord();
		factorArray[m_factorType] = factors[currPos];
	}

	// calc score on that phrase
	MyBase::CalcScore(phraseCalc, fullScore, ngramScore);
}


