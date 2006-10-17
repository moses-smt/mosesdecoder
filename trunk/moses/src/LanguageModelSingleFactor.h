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

#include "LanguageModel.h"
#include "Phrase.h"

class FactorCollection;
class Factor;

/* Abstract class which represent an implement of single factor LM
 * Currently inherited by SRI and IRST LM implmentations
 */
class LanguageModelSingleFactor : public LanguageModel
{
protected:	
	const Factor *m_sentenceStart, *m_sentenceEnd;
	FactorType	m_factorType;

	//! constructor to be called by inherited class
	LanguageModelSingleFactor(bool registerScore);

public:
	//! ??? if LM return this state when calculating score, then agressive pruning cannot be done
  static State UnknownState;

	virtual ~LanguageModelSingleFactor();
	//! load data from file
	virtual void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder) = 0;

	LMType GetLMType() const
	{
		return SingleFactor;
	}

	bool Useable(const Phrase &phrase) const
	{
		return (phrase.GetSize()>0 && phrase.GetFactor(0, m_factorType) != NULL);		
	}
	//! factor that represent sentence start. Usually <s>
	const Factor *GetSentenceStart() const
	{
		return m_sentenceStart;
	}
	//! factor that represent sentence start. Usually </s>
	const Factor *GetSentenceEnd() const
	{
		return m_sentenceEnd;
	}
	//! which factor type this LM uses
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
	const std::string GetScoreProducerDescription() const;
};

