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


void LanguageModel_IRST::Load(size_t id
												, const std::string &fileName
												, FactorCollection &factorCollection
												, FactorType factorType
												, float weight
												, size_t nGramOrder)
{
	m_id					 = id;
	m_factorType 	 = factorType;
	m_weight			 = weight;
	m_nGramOrder	 = nGramOrder;

	// TODO need to be able to configure these two parameters
	int resolution = 0;   // for quantized probs
	double decay   = 0.95;
  //

	InputFileStream inp(fileName);

	m_lmtb         = new lmtable(
		fileName.c_str(),
		nGramOrder,
		resolution,
		decay
	);
	// LM can be ok, just outputs warnings
	CreateFactors(factorCollection);		

}

void LanguageModel_IRST::CreateFactors(FactorCollection &factorCollection)
{ // add factors which have srilm id
	
	dict_entry *entry;
	LmId lmId;
	dictionary_iter iter(m_lmtb->dict);
	while ( (entry = iter.next()) != NULL)
	{
		LmId lmId;
    lmId.irst = entry->code;
		factorCollection.AddFactor(Output, m_factorType, entry->word, lmId);
	}
	
	lmId = GetLmID(BOS_);
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, SENTENCE_START, lmId);
	lmId = GetLmID(EOS_);
	m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, SENTENCE_END, lmId);
	
}

LmId LanguageModel_IRST::GetLmID( const std::string &str ) const
{
    LanguageModel_IRST *lm = const_cast<LanguageModel_IRST*>(this); // hack. not sure if supposed to cast this
    LmId res;
    res.sri = lm->m_lmtb->dict->encode( str.c_str() );
    return res;
}

float LanguageModel_IRST::GetValue(const vector<const Factor*> &contextFactor) const
{
	// set up context
	size_t count = contextFactor.size();
  ngram ng(m_lmtb->dict);
	for (size_t i = 0 ; i < count ; i++)
	{
#ifdef CDYER_DEBUG_LMSCORE
		std::cout << i <<"="<<contextFactor[i]->GetLmId().irst <<"," << contextFactor[i]->GetString()<<" ";
#endif
		ng.pushc(contextFactor[i]->GetLmId().irst);
	}
#ifdef CDYER_DEBUG_LMSCORE
	std::cout <<" (ng='" << ng << "')\n";
#endif
	return TransformScore(m_lmtb->prob(ng));
}

