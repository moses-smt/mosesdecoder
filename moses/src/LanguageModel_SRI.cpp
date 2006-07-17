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

#include "NGramNode.h"

#include "LanguageModel_SRI.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"

using namespace std;

LanguageModel_SRI::LanguageModel_SRI()
:m_srilmVocab()
,m_srilmModel(m_srilmVocab, 3)
{
	m_srilmModel.skipOOVs() = false;
}

void LanguageModel_SRI::Load(size_t id
												, const std::string &fileName
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_id					= id;
	m_factorType 	= factorType;
	m_weight			= weight;
	m_nGramOrder	= nGramOrder;

	File file( fileName.c_str(), "r" );
	if (m_srilmModel.read(file))
	{
	}
	else
	{
		TRACE_ERR("warning/failed loading language model" << endl);
	}
	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);		

}

void LanguageModel_SRI::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	
	VocabString str;
	LmId lmId;
	VocabIter iter(m_srilmVocab);
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

float LanguageModel_SRI::GetValue(const vector<const Factor*> &contextFactor) const
{
	// set up context
	size_t count = contextFactor.size();
	VocabIndex *context = (VocabIndex*) malloc(count * sizeof(VocabIndex));
	for (size_t i = 0 ; i < count - 1 ; i++)
	{
		context[i] = GetLmID(contextFactor[count-2-i]).sri;
	}
	context[count-1] = Vocab_None;
	
	// call sri lm fn
	float ret = GetValue(GetLmID(contextFactor[count-1]), context);
	free(context);
	return ret;
}

