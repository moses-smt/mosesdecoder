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
#include "FactorCollection.h"

template<typename LMImpl>
class LanguageModel_Chunking : public LanguageModelMultiFactor
{	
protected:
	FactorType m_posType, m_morphType, m_jointType;
	size_t m_realNGramOrder;
	LMImpl m_lmImpl;
	std::vector<std::string> m_posPrefix; // only process words with these tags
	mutable FactorCollection *m_factorCollection;
	
public:
	LanguageModel_Chunking(bool registerScore)
	: LanguageModelMultiFactor(registerScore)
	, m_lmImpl(false)
	{
		m_posPrefix.push_back("ART");
		m_posPrefix.push_back("P");
		m_posPrefix.push_back("V");
		m_posPrefix.push_back("$,");
		m_posPrefix.push_back("$.");
		m_posPrefix.push_back(BOS_);
		m_posPrefix.push_back(EOS_);
	}
	
	void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, const std::vector<FactorType> &factorTypes
					, float weight
					, size_t nGramOrder)
	{
		m_factorTypes 			= FactorMask(factorTypes);
		m_weight 						= weight;
		m_filename 					= fileName;
		m_nGramOrder 				= nGramOrder;
		m_factorCollection 	= &factorCollection;
		
		// hack. this LM is a joint factor of morph and POS tag
		m_posType						= 1;
		m_morphType					= 2;
		m_jointType 				= 3;
		m_realNGramOrder 		= 3;

		m_sentenceStartArray[m_posType] = factorCollection.AddFactor(Output, m_posType, BOS_);
		m_sentenceStartArray[m_morphType] = factorCollection.AddFactor(Output, m_morphType, BOS_);

		m_sentenceEndArray[m_posType] = factorCollection.AddFactor(Output, m_posType, EOS_);
		m_sentenceEndArray[m_morphType] = factorCollection.AddFactor(Output, m_morphType, EOS_);

		m_lmImpl.Load(fileName, factorCollection, m_jointType, weight, nGramOrder);
	}
		
	bool Process(const FactorArrayWrapper &factorArray) const
	{
		std::string str1stWord = factorArray[m_posType]->GetString();
		bool process = false;
		for (size_t i = 0 ; i < m_posPrefix.size() ; ++i)
		{
			if (str1stWord.find(m_posPrefix[i]) == 0)
			{
				process = true;
				break;
			}
		}
		return process;
	}
	
	float GetValue(const std::vector<FactorArrayWrapper> &contextFactor, State* finalState = NULL) const
	{
		if (contextFactor.size() == 0)
		{
			return 0;
		}
		/*
		for (size_t i = 0 ; i < contextFactor.size() ; ++i)
			TRACE_ERR(contextFactor[i] << " ");
		TRACE_ERR(std::endl);
		*/
		// only process context where last word is a verb, article, pronoun or comma or period
		if (!Process(contextFactor.back()))
			return 0;

		// create context in reverse 'cos we skip words we don't want
		std::vector<FactorArrayWrapper> chunkContext;
		for (int currPos = (int)contextFactor.size() - 1 ; currPos >= 0 && chunkContext.size() < m_realNGramOrder ; --currPos )
		{
			const FactorArrayWrapper &factorArray = contextFactor[currPos];
			bool process = Process(factorArray);
			if (!process)
				continue;

		// concatenate pos & morph factors and use normal LM to find prob
			std::string strConcate = factorArray[m_posType]->GetString() + "|" + factorArray[m_morphType]->GetString();
			
			const Factor *factor = m_factorCollection->AddFactor(Output, m_jointType, strConcate);
			Word chunkWord;
			chunkWord.SetFactor(m_jointType, factor);

			chunkContext.push_back(chunkWord);
		}
	
		// create context factor the right way round
		std::reverse(chunkContext.begin(), chunkContext.end());
		// calc score on chunked phrase

		/*
		for (size_t i = 0 ; i < chunkContext.size() ; ++i)
			TRACE_ERR(chunkContext[i] << " ");
		TRACE_ERR(std::endl);
		*/
		// calc score
		float ret = m_lmImpl.GetValue(chunkContext, finalState);
		
		return ret;
	}
};


