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

#include "NGramNode.h"

#include "LanguageModel.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "ScoreColl.h"

using namespace std;

// static variable init
const LmId LanguageModel::UNKNOWN_LM_ID(-1);

LanguageModel::LanguageModel() : m_unknownId(0) {}
LanguageModel::~LanguageModel() {}

// don't inline virtual funcs...
unsigned int LanguageModel::GetNumScoreComponents() const
{
	return 1;
}

const std::string LanguageModel::GetScoreProducerDescription() const
{
	std::ostringstream oss;
	// what about LMs that are over multiple factors at once, POS + stem, for example?
	oss << m_nGramOrder << "-gram LM score, factor-type=" << GetFactorType() << ", file=" << m_filename;
	return oss.str();
} 

/***
 * ngramComponent should be an invalid pointer iff n-best ranking is turned off
 */
void LanguageModel::CalcScore(const Phrase &phrase
														, float &fullScore
														, float &ngramScore) const
{
	fullScore	= 0;
	ngramScore	= 0;
	FactorType factorType = GetFactorType();

	size_t phraseSize = phrase.GetSize();
	vector<const Factor*> contextFactor;
	contextFactor.reserve(m_nGramOrder);

#undef CDYER_DEBUG_LMSCORE
#ifdef CDYER_DEBUG_LMSCORE
	std::cout<<"LM::CalcScore(" << phrase << "):\n";
#endif
		
	// start of sentence
	for (size_t currPos = 0 ; currPos < m_nGramOrder - 1 && currPos < phraseSize ; currPos++)
	{
		contextFactor.push_back(phrase.GetFactor(currPos, factorType));		
#ifdef CDYER_DEBUG_LMSCORE
    float score = GetValue(contextFactor);
		std::cout << "\t" << currPos << ": " << score << std::endl;
#endif
		fullScore += GetValue(contextFactor);
	}
	
	if (phraseSize >= m_nGramOrder)
	{
		contextFactor.push_back(phrase.GetFactor(m_nGramOrder - 1, factorType));
		ngramScore = GetValue(contextFactor);
#ifdef CDYER_DEBUG_LMSCORE
		std::cout << "\t" << m_nGramOrder-1 << " " << ngramScore << " (first full ngram)" << std::endl;
#endif
	}
	
	// main loop
	for (size_t currPos = m_nGramOrder; currPos < phraseSize ; currPos++)
	{ // used by hypo to speed up lm score calc
		for (size_t currNGramOrder = 0 ; currNGramOrder < m_nGramOrder - 1 ; currNGramOrder++)
		{
			contextFactor[currNGramOrder] = contextFactor[currNGramOrder + 1];
		}
		contextFactor[m_nGramOrder - 1] = phrase.GetFactor(currPos, factorType);
		float partScore = GetValue(contextFactor);
#ifdef CDYER_DEBUG_LMSCORE
		std::cout << "\t" << currPos << " " << partScore << " (full ngram)" << std::endl;
#endif
		
		ngramScore += partScore;		
	}
	fullScore += ngramScore;	
}
