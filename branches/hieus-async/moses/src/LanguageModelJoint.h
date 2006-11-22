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
#include <string>
#include <sstream>
#include "LanguageModelSingleFactor.h"
#include "LanguageModelMultiFactor.h"
#include "Word.h"
#include "FactorTypeSet.h"
#include "FactorCollection.h"

class Phrase;
class FactorCollection;

/** LM of multiple factors. A simple extension of single factor LM - factors backoff together.
 *	Rather slow as this uses string concatenation/split
*/
class LanguageModelJoint : public LanguageModelMultiFactor
{
protected:
	LanguageModelSingleFactor *m_lmImpl;
	FactorCollection *m_factorCollection;
	std::vector<FactorType> m_factorTypesOrdered;
	
	size_t m_implFactor;
public:
	LanguageModelJoint(LanguageModelSingleFactor *lmImpl, bool registerScore)
	:LanguageModelMultiFactor(registerScore)
	{
		m_lmImpl = lmImpl;
	}
	
	~LanguageModelJoint()
	{
		delete m_lmImpl;
	}
	
	bool Load(const std::string &filePath
					, FactorCollection &factorCollection
					, const std::vector<FactorType> &factorTypes
					, float weight
					, size_t nGramOrder)
	{
		m_factorTypes				= FactorMask(factorTypes);
		m_weight 						= weight;
		m_filePath 					= filePath;
		m_nGramOrder 				= nGramOrder;
	
		m_factorTypesOrdered= factorTypes;
		m_factorCollection	= &factorCollection;
		m_implFactor				= 0;
		
		// sentence markers
		for (size_t index = 0 ; index < factorTypes.size() ; ++index)
		{
			FactorType factorType = factorTypes[index];
			m_sentenceStartArray[factorType] 	= factorCollection.AddFactor(Output, factorType, BOS_);
			m_sentenceEndArray[factorType] 		= factorCollection.AddFactor(Output, factorType, EOS_);
		}
	
		return m_lmImpl->Load(filePath, factorCollection, m_implFactor, weight, nGramOrder);
	}
	
	float GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL, unsigned int* len = NULL) const
	{
		if (contextFactor.size() == 0)
		{
			return 0;
		}
		/*
		for (size_t i = 0 ; i < contextFactor.size() ; ++i)
			TRACE_ERR( contextFactor[i] << " ";
		TRACE_ERR( std::endl;
		*/

		// joint context for internal LM
		std::vector<const Word*> jointContext;
		
		for (size_t currPos = 0 ; currPos < m_nGramOrder ; ++currPos )
		{
			const Word &word = *contextFactor[currPos];

			// add word to chunked context
			std::stringstream stream("");

			const Factor *factor = word[ m_factorTypesOrdered[0] ];
			stream << factor->GetString();

			for (size_t index = 1 ; index < m_factorTypesOrdered.size() ; ++index)
			{
				FactorType factorType = m_factorTypesOrdered[index];
				const Factor *factor = word[factorType];
				stream << "|" << factor->GetString();
			}
			
			factor = m_factorCollection->AddFactor(Output, m_implFactor, stream.str());

			Word* jointWord = new Word;
			jointWord->SetFactor(m_implFactor, factor);
			jointContext.push_back(jointWord);
		}
	
		/*
		for (size_t i = 0 ; i < chunkContext.size() ; ++i)
			TRACE_ERR(chunkContext[i] << " ");
		TRACE_ERR(std::endl);
		*/
		// calc score on chunked phrase
		float ret = m_lmImpl->GetValue(jointContext, finalState, len);

		RemoveAllInColl(jointContext);
		
		return ret;
	}
	
};
