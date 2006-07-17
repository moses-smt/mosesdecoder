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

class FactorCollection;
class Factor;
class Phrase;

class LanguageModel
{
protected:	
	const Factor *m_sentenceStart, *m_sentenceEnd;
	FactorType	m_factorType;
	float				m_weight;
	size_t			m_id, m_nGramOrder;
public:

	static const LmId UNKNOWN_LM_ID;

	LanguageModel();
	virtual ~LanguageModel();
	virtual void Load(size_t id
					, const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder) = 0;
	
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
							, std::list< std::pair<size_t, float> >	&ngramComponent) const;
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
	virtual float GetValue(const std::vector<const Factor*> &contextFactor) const = 0;

  // one of the following should probably be made available
  // virtual LmId GetLmID( const Factor *factor )  const = 0;
  // virtual LmId GetLmID( const std::string &factor )  const = 0;
};

