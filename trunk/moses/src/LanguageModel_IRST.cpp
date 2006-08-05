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

#include "ngram.h"
#include "lmtable.h"
#include "dictionary.h"

#include "LanguageModel_IRST.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"

using namespace std;

LanguageModel_IRST::LanguageModel_IRST()
:m_lmtb(0)
{
}

LanguageModel_IRST::~LanguageModel_IRST()
{
  delete m_lmtb;
}


void LanguageModel_IRST::Load(const std::string &fileName
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_factorType 	 = factorType;
	m_weight			 = weight;
	m_nGramOrder	 = nGramOrder;
	m_filename		 = fileName;

	InputFileStream inp(fileName);

	m_lmtb         = new lmtable(inp);

	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);
  m_unknownId = m_lmtb->dict->oovcode();
  std::cerr << "IRST: m_unknownId=" << m_unknownId << std::endl;
}

void LanguageModel_IRST::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	// code copied & paste from SRI LM class. should do template function
	std::map<size_t, int> lmIdMap;
	size_t maxFactorId = 0; // to create lookup vector later on
	
	dict_entry *entry;
	dictionary_iter iter(m_lmtb->dict);
	while ( (entry = iter.next()) != NULL)
	{
		size_t factorId = factorCollection.AddFactor(Output, m_factorType, entry->word)->GetId();
		lmIdMap[factorId] = entry->code;
		maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	}
	
	size_t factorId;
	
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
	factorId = m_sentenceStart->GetId();
	lmIdMap[factorId] = GetLmID(BOS_);
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;

	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
	factorId = m_sentenceEnd->GetId();
	lmIdMap[factorId] = GetLmID(EOS_);;
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	
	// add to lookup vector in object
	m_lmIdLookup.resize(maxFactorId+1);
	
	fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

	map<size_t, int>::iterator iterMap;
	for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap)
	{
		m_lmIdLookup[iterMap->first] = iterMap->second;
	}
	
}

int LanguageModel_IRST::GetLmID( const std::string &str ) const
{
    return m_lmtb->dict->encode( str.c_str() );
}
int LanguageModel_IRST::GetLmID( const Factor *factor ) const
{
	size_t factorId = factor->GetId();
	return ( factorId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[factorId];
}

float LanguageModel_IRST::GetValue(const vector<const Factor*> &contextFactor, State* finalState) const
{
	// set up context
	size_t count = contextFactor.size();
  ngram ng(m_lmtb->dict);
	for (size_t i = 0 ; i < count ; i++)
	{
#undef CDYER_DEBUG_LMSCORE
#ifdef CDYER_DEBUG_LMSCORE
		std::cout << i <<"="<<contextFactor[i]->GetLmId().irst <<"," << contextFactor[i]->GetString()<<" ";
#endif
    int lmId = GetLmID(contextFactor[i]);
		ng.pushc(lmId);
	}
#ifdef CDYER_DEBUG_LMSCORE
	std::cout <<" (ng='" << ng << "')\n";
#endif
	if (finalState) {
		assert("!LM State needs to be implemented!");
	}
	return TransformScore(m_lmtb->prob(ng));
}

