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
#include "ScoreComponentCollection.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Dictionary.h"

class FactorCollection;

struct WordComparer
{
	//! returns true if hypoA can be recombined with hypoB
	bool operator()(const Word *a, const Word *b) const
	{
		return *a < *b;
	}
};

typedef std::map < Word , ScoreComponentCollection2 > OutputWordCollection;
		// 1st = output phrase
		// 2nd = log probability (score)

class GenerationDictionary : public Dictionary, public ScoreProducer
{
protected:
	std::map<const Word* , OutputWordCollection, WordComparer> m_collection;
	// 1st = source
	// 2nd = target
	std::string						m_filename;

public:
	GenerationDictionary(size_t numFeatures);
	virtual ~GenerationDictionary();

	DecodeType GetDecodeType() const
	{
		return Generate;
	}
	
	void Load(const std::vector<FactorType> &input
									, const std::vector<FactorType> &output
									, FactorCollection &factorCollection
									, const std::string &filePath
									, FactorDirection direction
									, bool forceSingleFeatureValue);

	unsigned int GetNumScoreComponents() const;
	const std::string GetScoreProducerDescription() const;

	size_t GetSize() const
	{
		return m_collection.size();
	}
	const OutputWordCollection *FindWord(const Word &word) const;
};

