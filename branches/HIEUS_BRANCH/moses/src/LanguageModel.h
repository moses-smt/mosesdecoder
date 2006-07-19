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

#ifdef LM_SRI
#include "Ngram.h"
#include "Vocab.h"
#endif

class FactorCollection;
class Factor;
class Phrase;
class ScoreColl;

class LanguageModel
{
protected:	
	const Factor *m_sentenceStart, *m_sentenceEnd;
	FactorType	m_factorType;
	float				m_weight;
	size_t			m_id, m_nGramOrder;
public:
	
	LanguageModel();
	void Load(size_t id
					, const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder);
	
	size_t GetNGramOrder() const
	{
		return m_nGramOrder;
	}
	const Factor *GetSentenceStart() const
	{
		return m_sentenceStart;
	}
	const Factor *GetSentenceEnd() const
	{
		return m_sentenceEnd;
	}
	void CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore
							, ScoreColl	*ngramComponent) const;
	FactorType GetFactorType() const
	{
		return m_factorType;
	}
	float GetWeight() const
	{
		return m_weight;
	}
	void SetWeight(float weight)
	{
		m_weight = weight;
	}
	size_t GetId() const
	{
		return m_id;
	}
	float GetValue(const std::vector<const Factor*> &contextFactor) const;
			
#ifdef LM_SRI
protected:
	Vocab m_srilmVocab;
	Ngram m_srilmModel;

	float GetValue(LmId wordId, LmId *context) const
	{
		LanguageModel *lm = const_cast<LanguageModel*>(this);	// hack. not sure if supposed to cast this
		float p = lm->m_srilmModel.wordProb( wordId, context );
		return FloorSRIScore(TransformSRIScore(p));	 // log10->log
	}

	LmId GetLmID( const Factor *factor )  const
	{
		return GetLmID(factor->GetString());
	}
	void CreateFactors(FactorCollection &factorCollection);
public:
	static const LmId UNKNOWN_LM_ID = 0;
	
	LmId GetLmID( const std::string &str ) const
	{
		LanguageModel *lm = const_cast<LanguageModel*>(this);	// hack. not sure if supposed to cast this
		return lm->m_srilmVocab.getIndex( str.c_str(), lm->m_srilmVocab.unkIndex() );
	}
	
#endif
#ifdef LM_INTERNAL

#define Vocab_None NULL;

protected:
	NGramCollection m_map;

	float GetValue(const Factor *factor0) const;
	float GetValue(const Factor *factor0, const Factor *factor1) const;
	float GetValue(const Factor *factor0, const Factor *factor1, const Factor *factor2) const;

public:
	static const LmId UNKNOWN_LM_ID;

	LmId GetLmID( const Factor *factor )  const
	{
		return factor->GetLmId();
	}
#endif
	
};

