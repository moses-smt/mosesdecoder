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

#include "Ngram.h"
#include "Vocab.h"

#include "LanguageModel_SRI.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"

using namespace std;

LanguageModel_SRI::LanguageModel_SRI()
:m_srilmVocab(0)
,m_srilmModel(0)
{
}

LanguageModel_SRI::~LanguageModel_SRI()
{
  delete m_srilmModel;
  delete m_srilmVocab;
}

void LanguageModel_SRI::Load(size_t id
												, const std::string &fileName
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_srilmVocab  = new Vocab();
  m_srilmModel	= new Ngram(*m_srilmVocab, nGramOrder);
	m_id					= id;
	m_factorType 	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;
	m_filename		= fileName;

	m_srilmModel->skipOOVs() = false;

	File file( fileName.c_str(), "r" );
	if (m_srilmModel->read(file))
	{
	}
	else
	{
		TRACE_ERR("warning/failed loading language model" << endl);
	}
	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);		
  m_unknownId.sri = m_srilmVocab->unkIndex();
}

void LanguageModel_SRI::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	
	VocabString str;
	LmId lmId;
	VocabIter iter(*m_srilmVocab);
	while ( (str = iter.next()) != NULL)
	{
		LmId lmId = GetLmID(str);
		factorCollection.AddFactor(Output, m_factorType, str, lmId);
	}
	
	lmId = GetLmID(SENTENCE_START);
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, SENTENCE_START, lmId);
	lmId = GetLmID(SENTENCE_END);
	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, SENTENCE_END, lmId);
}

LmId LanguageModel_SRI::GetLmID( const std::string &str ) const
{
    LmId res;
    res.sri = m_srilmVocab->getIndex( str.c_str(), m_unknownId.sri );
    return res;
}

float LanguageModel_SRI::GetValue(VocabIndex wordId, VocabIndex *context) const
{
	float p = m_srilmModel->wordProb( wordId, context );
	return FloorSRIScore(TransformSRIScore(p));  // log10->log
}

float LanguageModel_SRI::GetValue(const vector<const Factor*> &contextFactor) const
{
	// set up context
	VocabIndex context[MAX_NGRAM_SIZE];
	size_t count = contextFactor.size();
	for (size_t i = 0 ; i < count - 1 ; i++)
	{
    LmId x = contextFactor[count-2-i]->GetLmId();
		context[i] = x.sri==UNKNOWN_LM_ID.sri ? m_unknownId.sri : x.sri;
	}
	context[count-1] = Vocab_None;
	
	// call sri lm fn
  LmId x = contextFactor[count-1]->GetLmId();
	x.sri = x.sri==UNKNOWN_LM_ID.sri ? m_unknownId.sri : x.sri;
	float ret = GetValue(x.sri, context);
	return ret;
}

