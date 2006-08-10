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

#include "dictionary.h"
#include "n_gram.h"
#include "lmtable.h"


#include "LanguageModel_IRST.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"

using namespace std;

LanguageModel_IRST::LanguageModel_IRST(bool registerScore)
:LanguageModelSingleFactor(registerScore)
,m_lmtb(0)
{
}

LanguageModel_IRST::~LanguageModel_IRST()
{
  delete m_lmtb;
  delete m_lmtb_ng;
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

  //Marcello: modification to fit new version of lmtable.cpp
	//temporary replacement for: InputFileStream inp(fileName);
  std::fstream inp(fileName.c_str(),std::ios::in);  
	m_lmtb  = new lmtable;
  m_lmtb->load(inp);
  
  m_lmtb_ng=new ngram(m_lmtb->dict);
  m_lmtb_size=m_lmtb->maxlevel();
  
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
	m_lmtb_sentenceStart=lmIdMap[factorId] = GetLmID(BOS_);
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	m_sentenceStartArray[m_factorType] = m_sentenceStart;

	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
	factorId = m_sentenceEnd->GetId();
	m_lmtb_sentenceEnd=lmIdMap[factorId] = GetLmID(EOS_);
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	m_sentenceEndArray[m_factorType] = m_sentenceEnd;
	
	// add to lookup vector in object
	m_lmIdLookup.resize(maxFactorId+1);
	
	fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

	map<size_t, int>::iterator iterMap;
	for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap)
	{
		m_lmIdLookup[iterMap->first] = iterMap->second;
	}
	
  //initialize cache memory
  m_lmtb->init_prcache();
}

int LanguageModel_IRST::GetLmID( const std::string &str ) const
{
    return m_lmtb->dict->encode( str.c_str() );
}

float LanguageModel_IRST::GetValue(const vector<FactorArrayWrapper> &contextFactor, State* finalState) const
{
	FactorType factorType = GetFactorType();
	
	// set up context
	size_t count = contextFactor.size();
    
  m_lmtb_ng->size=0;
  if (count< (m_lmtb_size-1)) m_lmtb_ng->pushc(m_lmtb_sentenceEnd);
  if (count< m_lmtb_size) m_lmtb_ng->pushc(m_lmtb_sentenceStart);  
  
	for (size_t i = 0 ; i < count ; i++)
	{

    int lmId = GetLmID(contextFactor[i][factorType]);
    m_lmtb_ng->pushc(lmId);
	}
  
	if (finalState){        
    *finalState=(State *)m_lmtb->cmaxsuffptr(*m_lmtb_ng);		
	}

	//return TransformIRSTScore(m_lmtb->clprob(*m_lmtb_ng));
  return TransformIRSTScore(m_lmtb->clprob(*m_lmtb_ng));
}

