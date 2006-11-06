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
#include <fstream>
#include "dictionary.h"
#include "n_gram.h"
#include "lmtable.h"

#include "LanguageModelIRST.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"

using namespace std;

LanguageModelIRST::LanguageModelIRST(bool registerScore)
:LanguageModelSingleFactor(registerScore)
,m_lmtb(0)
{
}

LanguageModelIRST::~LanguageModelIRST()
{
  delete m_lmtb;
  delete m_lmtb_ng;
}


void LanguageModelIRST::Load(const std::string &filePath
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_factorType 	 = factorType;
	m_weight			 = weight;
	m_nGramOrder	 = nGramOrder;
	m_filePath		 = filePath;

  // Open the input file (possibly gzipped) and load the (possibly binary) model
	InputFileStream inp(filePath);
	m_lmtb  = new lmtable;

	m_lmtb->load(inp,filePath.c_str(),1);

  m_lmtb_ng=new ngram(m_lmtb->dict);
  m_lmtb_size=m_lmtb->maxlevel();
  
	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);
  m_unknownId = m_lmtb->dict->oovcode();
  std::cerr << "IRST: m_unknownId=" << m_unknownId << std::endl;
  
  //install caches
  m_lmtb->init_probcache();
  m_lmtb->init_statecache();
  m_lmtb->init_lmtcaches(m_lmtb->maxlevel()>2?m_lmtb->maxlevel()-1:2);
}

void LanguageModelIRST::CreateFactors(FactorCollection &factorCollection)
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
  
  
}

int LanguageModelIRST::GetLmID( const std::string &str ) const
{
    return m_lmtb->dict->encode( str.c_str() );
}

float LanguageModelIRST::GetValue(const vector<const Word*> &contextFactor, State* finalState, unsigned int* len) const
{
	unsigned int dummy;
	if (!len) { len = &dummy; }
	FactorType factorType = GetFactorType();
	
	// set up context
	size_t count = contextFactor.size();
    
	m_lmtb_ng->size=0;
	if (count< (size_t)(m_lmtb_size-1)) m_lmtb_ng->pushc(m_lmtb_sentenceEnd);
	if (count< (size_t)m_lmtb_size) m_lmtb_ng->pushc(m_lmtb_sentenceStart);  
  
	for (size_t i = 0 ; i < count ; i++)
	{

		int lmId = GetLmID((*contextFactor[i])[factorType]);
		m_lmtb_ng->pushc(lmId);
	}
  
	if (finalState){        
		*finalState=(State *)m_lmtb->cmaxsuffptr(*m_lmtb_ng);	
		// back off stats not currently available
		*len = 0;	
	}

	return TransformIRSTScore((float) m_lmtb->clprob(*m_lmtb_ng));
}


const void LanguageModelIRST::CleanUpAfterSentenceProcessing(){
  cerr << "reset caches and mmap\n";
  m_lmtb->reset_caches();  
  m_lmtb->reset_mmap();
}

const void LanguageModelIRST::InitializeBeforeSentenceProcessing(){
  //nothing to do
#ifdef TRACE_CACHE
 m_lmtb->sentence_id++;
#endif
}

