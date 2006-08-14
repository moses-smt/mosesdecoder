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

#include <string>
#include <vector>
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"
#include "LanguageModelSingleFactor.h"

class FactorCollection;
class Factor;
class Phrase;

class lmtable;  // irst lm table
class ngram;

class LanguageModel_IRST : public LanguageModelSingleFactor
{
protected:
	std::vector<int> m_lmIdLookup;
	lmtable* m_lmtb;
  ngram* m_lmtb_ng;
  
	int	m_unknownId;
  int m_lmtb_sentenceStart; //lmtb symbols to initialize ngram with
  int m_lmtb_sentenceEnd;   //lmt symbol to initialize ngram with 
	int m_lmtb_size;          //max ngram stored in the table

  
//	float GetValue(LmId wordId, ngram *context) const;

	void CreateFactors(FactorCollection &factorCollection);
	int GetLmID( const std::string &str ) const;

	int GetLmID( const Factor *factor ) const{
    size_t factorId = factor->GetId();
    return ( factorId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[factorId];        
  };
  
public:
	LanguageModel_IRST(bool registerScore);
	~LanguageModel_IRST();
	void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);

  virtual float GetValue(const std::vector<FactorArrayWrapper> &contextFactor, State* finalState = NULL, unsigned int* len) const;

  const void CleanUpAfterSentenceProcessing();
  const void InitializeBeforeSentenceProcessing();
};
