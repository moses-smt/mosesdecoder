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
#include "NGramNode.h"
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"

#include "LanguageModel.h"
#include "Ngram.h"
#include "Vocab.h"


class FactorCollection;
class Factor;
class Phrase;

class LanguageModel_SRI : public LanguageModel
{
public:
	
	LanguageModel_SRI();
	void Load(size_t id
					, const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	
protected:
	Vocab m_srilmVocab;  // TODO - make this a ptr, remove #include from header
	Ngram m_srilmModel;  // "  "

	float GetValue(LmId wordId, VocabIndex *context) const
	{
		LanguageModel_SRI *lm = const_cast<LanguageModel_SRI*>(this);	// hack. not sure if supposed to cast this
		float p = lm->m_srilmModel.wordProb( wordId.sri, context );
		return FloorSRIScore(TransformSRIScore(p));	 // log10->log
	}

	LmId GetLmID( const Factor *factor )  const
	{
		return GetLmID(factor->GetString());
	}
	void CreateFactors(FactorCollection &factorCollection);
public:
	static const LmId UNKNOWN_LM_ID;
	
	LmId GetLmID( const std::string &str ) const
	{
		LanguageModel_SRI *lm = const_cast<LanguageModel_SRI*>(this);	// hack. not sure if supposed to cast this
		LmId res;
    res.sri = lm->m_srilmVocab.getIndex( str.c_str(), lm->m_srilmVocab.unkIndex() );
    return res;
	}

  virtual float GetValue(const vector<const Factor*> &contextFactor) const;
};

