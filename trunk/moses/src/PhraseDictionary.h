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

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "Phrase.h"
#include "TargetPhrase.h"

typedef std::list < TargetPhrase > TargetPhraseCollection;

class PhraseDictionary
{
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionary&);

protected:
	const size_t m_id, m_noScoreComponent;
	std::map<Phrase , TargetPhraseCollection > m_collection;
	// 1st = source
	// 2nd = target

	std::vector< FactorTypeSet* > m_factorsUsed;
		// should index this by language

	void AddEquivPhrase(const Phrase &source
											, const TargetPhrase &targetPhrase
											, size_t maxTargetPhrase);
	bool Contains(const std::vector< std::vector<std::string> >	&phraseVector
							, const std::list<Phrase>					&inputPhraseList
							, const std::vector<FactorType>		&inputFactorType);
public:
	PhraseDictionary(size_t id, size_t noScoreComponent)
		:m_id(id)
		,m_factorsUsed(2)
		,m_noScoreComponent(noScoreComponent)
	{
	}
	~PhraseDictionary();

	void Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, FactorCollection &factorCollection
								, const std::string &filePath
								, const std::string &hashFilePath
								, const std::vector<float> &weight
								, size_t maxTargetPhrase
								, bool filter
								, const std::list< Phrase > &inputPhraseList);
	
	const FactorTypeSet &GetFactorsUsed(const Language &language) const
	{
		return *m_factorsUsed[language];
	}
	size_t GetSize() const
	{
		return m_collection.size();
	}
	size_t GetNoScoreComponent() const
	{
		return m_noScoreComponent;
	}
	const TargetPhraseCollection *FindEquivPhrase(const Phrase &source) const;

	// for mert
	void SetWeightTransModel(const std::vector<float> &weightT);
	size_t GetId() const
	{
		return m_id;
	}
};

