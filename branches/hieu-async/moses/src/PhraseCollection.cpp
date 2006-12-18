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

#include "PhraseCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"

using namespace std;

PhraseCollectionNode *PhraseCollectionNode::GetOrCreateChild(const Word &word, bool initValue)
{
	NodeMap::iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// can't find node. create a new 1
	PhraseCollectionNode *node = &(m_map[word] = PhraseCollectionNode());
	node->m_value = initValue;

	return node;
}

const PhraseCollectionNode *PhraseCollectionNode::Find(const Word &word) const
{
	NodeMap::const_iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// don't return anything
	return NULL;
}

void PhraseCollection::AddPhrase(const Phrase &source)
{
	const size_t size = source.GetSize();
	
	PhraseCollectionNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetOrCreateChild(word, true);
		assert (currNode != NULL);
	}

	currNode->Set(true);

}

PhraseCollection::PhraseCollection(std::string filePath, FactorCollection &factorCollection)
{
	InputFileStream inFile(filePath);
	
	const std::vector<FactorType> &factorOrder = StaticData::Instance()->GetInputFactorOrder();
	const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();

	string line;
	while(getline(inFile, line)) 
	{
		Phrase phrase(Input);
		phrase.CreateFromString( factorOrder
													, line
													, factorCollection
													, factorDelimiter
													, NULL);
		AddPhrase(phrase);

		// add all suffixes too
		for (size_t startPos = 1 ; startPos < phrase.GetSize() ; ++startPos)
		{
			Phrase subPhrase = phrase.GetSubString(WordsRange(NOT_FOUND, startPos, phrase.GetSize()));
			AddPhrase(subPhrase);
		}
	}
}

bool PhraseCollection::Find(const Phrase &source, bool notFoundValue) const
{
	const size_t size = source.GetSize();
	
	const PhraseCollectionNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->Find(word);
		if (currNode == NULL)
			return notFoundValue;
	}

	return currNode->Get();

}