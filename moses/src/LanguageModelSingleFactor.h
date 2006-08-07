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

class FactorCollection;
class Factor;
class Phrase;

class LanguageModelSingleFactor : public LanguageModel
{
protected:	
	const Factor *m_sentenceStart, *m_sentenceEnd;
	FactorArray m_sentenceStartArray, m_sentenceEndArray;
	FactorType	m_factorType;
public:
  static State UnknownState;

	LanguageModelSingleFactor();
	virtual ~LanguageModelSingleFactor();
	virtual void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder) = 0;

	const Factor *GetSentenceStart() const
	{
		return m_sentenceStart;
	}
	const Factor *GetSentenceEnd() const
	{
		return m_sentenceEnd;
	}
	const FactorArray &GetSentenceStartArray() const
	{
		return m_sentenceStartArray;
	}
	const FactorArray &GetSentenceEndArray() const
	{
		return m_sentenceEndArray;
	}
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

