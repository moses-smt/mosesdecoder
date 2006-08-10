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
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"
#include "ScoreProducer.h"
#include "Word.h"

class FactorCollection;
class Factor;
class Phrase;

class LanguageModel : public ScoreProducer
{
protected:	
	float				m_weight;
	std::string	m_filename;
	size_t			m_nGramOrder;
	FactorArray m_sentenceStartArray, m_sentenceEndArray;
public:
  typedef const void* State;

	LanguageModel(bool registerScore);
	virtual ~LanguageModel();

	// see ScoreProducer.h
	unsigned int GetNumScoreComponents() const;

	virtual LMType GetLMType() const = 0;

	// whether this LM can be used on a particular phrase
	virtual bool Useable(const Phrase &phrase) const = 0;

	void CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore) const;
	virtual float GetValue(const std::vector<FactorArrayWrapper> &contextFactor, State* finalState = NULL) const = 0;
	State GetState(const std::vector<FactorArrayWrapper> &contextFactor) const;

	size_t GetNGramOrder() const
	{
		return m_nGramOrder;
	}
	const FactorArray &GetSentenceStartArray() const
	{
		return m_sentenceStartArray;
	}
	const FactorArray &GetSentenceEndArray() const
	{
		return m_sentenceEndArray;
	}
	
	float GetWeight() const
	{
		return m_weight;
	}
	void SetWeight(float weight)
	{
		m_weight = weight;
	}
	virtual const std::string GetScoreProducerDescription() const = 0;
};

