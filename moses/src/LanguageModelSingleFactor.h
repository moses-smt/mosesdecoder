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
	FactorType	m_factorType;
public:
  typedef const void* State;
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
	virtual void CalcScore(const Phrase &phrase
											, float &fullScore
											, float &ngramScore) const;
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
	virtual float GetValue(const std::vector<const Factor*> &contextFactor, State* finalState = 0) const = 0;

	LanguageModelSingleFactor::State GetState(const std::vector<const Factor*> &contextFactor) const;

  // one of the following should probably be made available
  // virtual LmId GetLmID( const Factor *factor )  const = 0;
  // virtual LmId GetLmID( const std::string &factor )  const = 0;
};

