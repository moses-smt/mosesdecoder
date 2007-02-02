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

#include "PrefixPhraseCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "PhraseList.h"

using namespace std;

PrefixPhraseCollectionNode *PrefixPhraseCollectionNode::GetOrCreateChild(const Word &word, bool initValue)
{
	NodeMap::iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// can't find node. create a new 1
	PrefixPhraseCollectionNode *node = &(m_map[word] = PrefixPhraseCollectionNode());
	node->m_value = initValue;

	return node;
}

const PrefixPhraseCollectionNode *PrefixPhraseCollectionNode::Find(const Word &word) const
{
	NodeMap::const_iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// don't return anything
	return NULL;
}

PrefixPhraseCollection::PrefixPhraseCollection(const std::vector<FactorType> &input
																			, const PhraseList &phraseList)
:m_inputMask(input)
{
	PhraseList::const_iterator iterList;
	for (iterList = phraseList.begin() ; iterList != phraseList.end() ; ++iterList)
	{
		const Phrase &phrase = *iterList;
		AddPhrase(phrase);

		// add all suffixes too
		for (size_t startPos = 1 ; startPos < phrase.GetSize() ; ++startPos)
		{
			Phrase subPhrase = phrase.GetSubString(WordsRange(NOT_FOUND, startPos, phrase.GetSize()));
			AddPhrase(subPhrase);
		}
	}
}

void PrefixPhraseCollection::AddPhrase(const Phrase &source)
{
	const size_t size = source.GetSize();
	
	PrefixPhraseCollectionNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		Word word = source.GetWord(pos);
		word.TrimFactors(m_inputMask);
		currNode = currNode->GetOrCreateChild(word, true);
		assert (currNode != NULL);
	}

	currNode->Set(true);

}

bool PrefixPhraseCollection::Find(const Phrase &source, bool notFoundValue) const
{
	const size_t size = source.GetSize();
	
	const PrefixPhraseCollectionNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->Find(word);
		if (currNode == NULL)
			return notFoundValue;
	}

	return currNode->Get();

}


