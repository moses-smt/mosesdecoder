// $Id$
// vim:tabstop=2

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

#include <map>
#include <vector>
#include <iterator>
#include "Word.h"
#include "TargetPhraseCollection.h"
#include "PhraseDictionaryNode.h"

namespace Moses
{

class PhraseDictionaryMemory;
class PhraseDictionaryNewFormat;

/** One node of the PhraseDictionaryMemory structure
*/
class PhraseDictionaryNodeMemory : public PhraseDictionaryNode
{
	typedef std::map<Word, PhraseDictionaryNodeMemory> NodeMap;

	// only these classes are allowed to instantiate this class
	friend class PhraseDictionaryMemory;
	friend class std::map<Word, PhraseDictionaryNodeMemory>;

protected:
	NodeMap m_map;
	TargetPhraseCollection *m_targetPhraseCollection;
	const Word *m_sourceWord;

	PhraseDictionaryNodeMemory()
		:m_targetPhraseCollection(NULL)
		,m_sourceWord(NULL)
	{}
public:
	~PhraseDictionaryNodeMemory();

	void CleanUp();
	void Sort(size_t tableLimit);
	PhraseDictionaryNodeMemory *GetOrCreateChild(const Word &word);
	const PhraseDictionaryNodeMemory *GetChild(const Word &word) const;
	const TargetPhraseCollection *GetTargetPhraseCollection() const
	{
		return m_targetPhraseCollection;
	}
	TargetPhraseCollection &GetOrCreateTargetPhraseCollection()
	{
		if (m_targetPhraseCollection == NULL)
			m_targetPhraseCollection = new TargetPhraseCollection();
		return *m_targetPhraseCollection;
	}
	size_t GetSize() const
	{ return m_map.size(); }

	// for mert
	void SetWeightTransModel(const PhraseDictionary *phraseDictionary
													, const std::vector<float> &weightT);

	const Word &GetSourceWord() const
	{ return *m_sourceWord; }
	void SetSourceWord(const Word &sourceWord)
	{ m_sourceWord = &sourceWord; }

	// iterators
	typedef NodeMap::iterator iterator;
	typedef NodeMap::const_iterator const_iterator;
	const_iterator begin() const { return m_map.begin(); }
	const_iterator end() const { return m_map.end(); }
	iterator begin() { return m_map.begin(); }
	iterator end() { return m_map.end(); }
	
	TO_STRING();
};

}
