// $Id: PhraseCollection.cpp 1083 2006-12-17 13:07:46Z hieuhoang1972 $
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

#include <map>
#include <string>
#include "Word.h"
#include "FactorMask.h"

class Phrase;
class PhraseList;

class PrefixPhraseCollectionNode
{
protected:
	typedef std::map<Word, PrefixPhraseCollectionNode> NodeMap;
	NodeMap m_map;
	bool m_value;
public:
	PrefixPhraseCollectionNode *GetOrCreateChild(const Word &word, bool initValue);
	const PrefixPhraseCollectionNode *Find(const Word &word) const;
	bool Get() const
	{
		return m_value;
	}
	void Set(bool value)
	{
		m_value = value;
	}
};

/** prefix tree of phrases. Used to store input phrases for filtering */
class PrefixPhraseCollection
{
protected:
	FactorMask m_inputMask;
	PrefixPhraseCollectionNode m_collection;
	//! Add source phrase and all prefix strings
	void AddPhrase(const Phrase &source);
public:
	PrefixPhraseCollection(const std::vector<FactorType> &input
									,const PhraseList &phraseList);
	bool Find(const Phrase &source, bool notFoundValue) const;

};

