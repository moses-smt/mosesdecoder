// $Id: PhraseDictionaryNodeNewFormat.h 3049 2010-04-05 18:34:09Z hieuhoang1972 $
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
class ChartRuleCollection;
class CellCollection;
class InputType;

	
/** One node of the PhraseDictionaryMemory structure
*/
class PhraseDictionaryNodeNewFormat : public PhraseDictionaryNode
{
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryNodeNewFormat&);
	
	typedef std::map<Word, PhraseDictionaryNodeNewFormat> InnerNodeMap;
	typedef std::map<Word, InnerNodeMap> NodeMap;
		// 1st word = source side non-term, or the word if term
		// 2nd word = target side non term, or the word if term

	// only these classes are allowed to instantiate this class
	friend class PhraseDictionaryNewFormat;
	friend class std::map<Word, PhraseDictionaryNodeNewFormat>;
	
protected:
	static size_t s_id;
	size_t m_id; // used for backoff	
	NodeMap m_map;
	mutable TargetPhraseCollection *m_targetPhraseCollection;
	const Word *m_sourceWord;
	float m_entropy;
	
	PhraseDictionaryNodeNewFormat()
		:m_id(s_id++)
		,m_targetPhraseCollection(NULL)
		,m_sourceWord(NULL)
	{}
public:
	virtual ~PhraseDictionaryNodeNewFormat();

	void CleanUp();
	void Sort(size_t tableLimit);
	PhraseDictionaryNodeNewFormat *GetOrCreateChild(const Word &word, const Word &sourcelabel);
	const PhraseDictionaryNodeNewFormat *GetChild(const Word &word, const Word &sourcelabel) const;
	
	const TargetPhraseCollection *GetTargetPhraseCollection() const
	{	return m_targetPhraseCollection; }
	TargetPhraseCollection &GetOrCreateTargetPhraseCollection()
	{
		if (m_targetPhraseCollection == NULL)
			m_targetPhraseCollection = new TargetPhraseCollection();
		return *m_targetPhraseCollection;
	}
	size_t GetSize() const
	{ return m_map.size(); }
	size_t GetId() const
	{ return m_id; }
	void SetId(size_t id)
	{ m_id = id; }
	float GetEntropy() const
	{ return m_entropy; }
	void SetEntropy(float entropy)
	{ m_entropy = entropy; }

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
