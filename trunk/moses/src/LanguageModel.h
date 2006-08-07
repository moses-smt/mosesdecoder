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

class FactorCollection;
class Factor;
class Phrase;

class LanguageModel : public ScoreProducer
{
protected:	
	float				m_weight;
	std::string	m_filename;
	size_t			m_nGramOrder;
public:
  typedef const void* State;

	LanguageModel();
	virtual ~LanguageModel();
	virtual void Load(const std::string &fileName
					, FactorCollection &factorCollection
					, FactorType factorType
					, float weight
					, size_t nGramOrder) = 0;

	// see ScoreProducer.h
	unsigned int GetNumScoreComponents() const;

	void CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore) const;
	virtual float GetValue(const std::vector<const FactorArray*> &contextFactor, State* finalState = NULL) const = 0;
	State GetState(const std::vector<const FactorArray*> &contextFactor) const;

	size_t GetNGramOrder() const
	{
		return m_nGramOrder;
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

