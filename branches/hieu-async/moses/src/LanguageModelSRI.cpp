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
#include "Ngram.h"
#include "Vocab.h"

#include "LanguageModelSRI.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
using namespace std;

LanguageModelSRI::LanguageModelSRI(bool registerScore, ScoreIndexManager &scoreIndexManager)
:LanguageModelSingleFactor(registerScore, scoreIndexManager)
, m_srilmVocab(0)
, m_srilmModel(0)
{
}

LanguageModelSRI::~LanguageModelSRI()
{
  delete m_srilmModel;
  delete m_srilmVocab;
}

bool LanguageModelSRI::Load(const std::string &filePath
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_srilmVocab  = new Vocab();
  m_srilmModel	= new Ngram(*m_srilmVocab, nGramOrder);
	m_factorType 	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;
	m_filePath		= filePath;

	m_srilmModel->skipOOVs() = false;

	File file( filePath.c_str(), "r" );

	// read file, no error catching
	m_srilmModel->read(file);

	// LM can be ok, just outputs warnings
	CreateFactors();		
  m_unknownId = m_srilmVocab->unkIndex();
  
  return true;
}

void LanguageModelSRI::CreateFactors()
{ // add factors which have srilm id
	
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	std::map<size_t, VocabIndex> lmIdMap;
	size_t maxFactorId = 0; // to create lookup vector later on
	
	VocabString str;
	VocabIter iter(*m_srilmVocab);
	while ( (str = iter.next()) != NULL)
	{
		VocabIndex lmId = GetLmID(str);
		size_t factorId = factorCollection.AddFactor(Output, m_factorType, str)->GetId();
		lmIdMap[factorId] = lmId;
		maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	}
	
	size_t factorId;
	
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
	factorId = m_sentenceStart->GetId();
	lmIdMap[factorId] = GetLmID(BOS_);
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	m_sentenceStartArray[m_factorType] = m_sentenceStart;
	
	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
	factorId = m_sentenceEnd->GetId();
	lmIdMap[factorId] = GetLmID(EOS_);
	maxFactorId = (factorId > maxFactorId) ? factorId : maxFactorId;
	m_sentenceEndArray[m_factorType] = m_sentenceEnd;
	
	// add to lookup vector in object
	m_lmIdLookup.resize(maxFactorId+1);
	
	fill(m_lmIdLookup.begin(), m_lmIdLookup.end(), m_unknownId);

	map<size_t, VocabIndex>::iterator iterMap;
	for (iterMap = lmIdMap.begin() ; iterMap != lmIdMap.end() ; ++iterMap)
	{
		m_lmIdLookup[iterMap->first] = iterMap->second;
	}
}

VocabIndex LanguageModelSRI::GetLmID( const std::string &str ) const
{
    return m_srilmVocab->getIndex( str.c_str(), m_unknownId );
}
VocabIndex LanguageModelSRI::GetLmID( const Factor *factor ) const
{
	size_t factorId = factor->GetId();
	return ( factorId >= m_lmIdLookup.size()) ? m_unknownId : m_lmIdLookup[factorId];
}

float LanguageModelSRI::GetValue(VocabIndex wordId, VocabIndex *context) const
{
	float p = m_srilmModel->wordProb( wordId, context );
	return FloorScore(TransformSRIScore(p));  // log10->log
}

float LanguageModelSRI::GetValue(const vector<Word> &contextFactor, State* finalState, unsigned int *len) const
{
	FactorType	factorType = GetFactorType();
	size_t count = contextFactor.size();
	if (count <= 0)
	{
		finalState = NULL;
		return 0;
	}
		
	// set up context
	VocabIndex context[MAX_NGRAM_SIZE];
	for (size_t i = 0 ; i < count - 1 ; i++)
	{
		context[i] =  GetLmID((contextFactor[count-2-i])[factorType]);
	}
	context[count-1] = Vocab_None;
	
	assert((contextFactor[count-1])[factorType] != NULL);
	// call sri lm fn
	VocabIndex lmId= GetLmID((contextFactor[count-1])[factorType]);
	float ret = GetValue(lmId, context);

	if (finalState) {
		for (int i = count - 2 ; i >= 0 ; i--)
			context[i+1] = context[i];
		context[0] = lmId;
		unsigned int dummy;
		if (!len) { len = &dummy; }
		*finalState = m_srilmModel->contextID(context,*len);
		(*len)++;
	}
	return ret;
}


