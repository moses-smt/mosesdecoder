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

#include <list>
#include <map>
#include <vector>
#include "Phrase.h"
#include "TypeDef.h"
#include "Dictionary.h"

class FactorCollection;

typedef std::map < Word , float > OutputWordCollection;
		// 1st = output phrase
		// 2nd = log probability (score)

class GenerationDictionary : public Dictionary, public ScoreProducer
{
protected:
	std::map<Word , OutputWordCollection> m_collection;
	// 1st = source
	// 2nd = target
	OutputWordCollection	m_unknownWord;
	float									m_weight;
	std::string						m_filename;

public:
	GenerationDictionary()
		: Dictionary(1)
	{
	}
	virtual ~GenerationDictionary();

	DecodeType GetDecodeType() const
	{
		return Generate;
	}
	
	void Load(const std::vector<FactorType> &input
									, const std::vector<FactorType> &output
									, FactorCollection &factorCollection
									, const std::string &filePath
									, float weight
									, FactorDirection direction);

	unsigned int GetNumScoreComponents() const;
	const std::string GetScoreProducerDescription() const;

	float GetWeight() const
	{
		return m_weight;
	}
	size_t GetSize() const
	{
		return m_collection.size();
	}
	void SetWeight(float weight)
	{
		m_weight = weight;
	}
	const OutputWordCollection *FindWord(const FactorArray &factorArray) const;
};

