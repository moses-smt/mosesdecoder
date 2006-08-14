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

class LanguageModel_Chunking : public LanguageModelSingleFactor
{	
protected:
	size_t m_realNGramOrder;
	LanguageModelSingleFactor *m_lmImpl;
	
public:
	LanguageModel_Chunking(LanguageModelSingleFactor *lmImpl, bool registerScore)
	: LanguageModelSingleFactor(registerScore)
	{
		m_lmImpl = lmImpl;		
	}
	~LanguageModel_Chunking()
	{
		delete m_lmImpl;
	}
	void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder)
	{
		m_factorType 				= factorType;
		m_weight 						= weight;
		m_filename 					= fileName;
		m_nGramOrder 				= nGramOrder;
		
		// hack. this LM is a joint factor of morph and low POS tag & hacked-up TIGER tag 
		m_realNGramOrder 		= 3;

		m_sentenceStartArray[m_factorType] = factorCollection.AddFactor(Output, m_factorType, BOS_);
		m_sentenceEndArray[m_factorType] = factorCollection.AddFactor(Output, m_factorType, EOS_);

		m_lmImpl->Load(fileName, factorCollection, m_factorType, weight, nGramOrder);
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
		// only process context where last word is a word we want
		const Factor *factor = contextFactor.back()[m_factorType];
		std::string strWord = factor->GetString();
		if (strWord.find("???") == 0)
			return 0;
		
		// add last word
		std::vector<FactorArrayWrapper> chunkContext;
		Word chunkWord;
		chunkWord.SetFactor(m_factorType, factor);
		chunkContext.push_back(chunkWord);
		
		// create context in reverse 'cos we skip words we don't want
		for (int currPos = (int)contextFactor.size() - 2 ; currPos >= 0 && chunkContext.size() < m_realNGramOrder ; --currPos )
		{
			const FactorArrayWrapper &factorArray = contextFactor[currPos];
			factor = factorArray[m_factorType];
			std::string strWord = factor->GetString();
			bool skip = strWord.find("???") == 0;
			if (skip)
				continue;

			// add word to chunked context
			Word chunkWord;
			chunkWord.SetFactor(m_factorType, factor);
			chunkContext.push_back(chunkWord);
		}
	
		// create context factor the right way round
		std::reverse(chunkContext.begin(), chunkContext.end());
		/*
		for (size_t i = 0 ; i < chunkContext.size() ; ++i)
			TRACE_ERR(chunkContext[i] << " ");
		TRACE_ERR(std::endl);
		*/
		// calc score on chunked phrase
		float ret = m_lmImpl->GetValue(chunkContext, finalState);
		
		return ret;
	}
};


