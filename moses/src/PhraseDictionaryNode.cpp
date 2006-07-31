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

#include "PhraseDictionaryNode.h"
#include "TargetPhrase.h"
#include "PhraseDictionary.h"

PhraseDictionaryNode *PhraseDictionaryNode::GetOrCreateChild(const Word word)
{
	NodeMap::iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// can't find node. create a new 1
	return &(m_map[word] = PhraseDictionaryNode());
}

const PhraseDictionaryNode *PhraseDictionaryNode::GetChild(const Word word) const
{
	NodeMap::const_iterator iter = m_map.find(word);
	if (iter != m_map.end())
		return &iter->second;	// found it

	// don't return anything
	return NULL;
}

void PhraseDictionaryNode::SetWeightTransModel(const PhraseDictionary *phraseDictionary
																							 , const std::vector<float> &weightT)
{
	// recursively set weights
	NodeMap::iterator iterNodeMap;
	for (iterNodeMap = m_map.begin() ; iterNodeMap != m_map.end() ; ++iterNodeMap)
	{
		iterNodeMap->second.SetWeightTransModel(phraseDictionary, weightT);
	}

	// set wieghts for this target phrase
	if (m_targetPhraseCollection == NULL)
		return;

	TargetPhraseCollection::iterator iterTargetPhrase;
	for (iterTargetPhrase = m_targetPhraseCollection->begin();
				iterTargetPhrase != m_targetPhraseCollection->end();
				++iterTargetPhrase)
	{
		TargetPhrase &targetPhrase = *iterTargetPhrase;
		targetPhrase.SetWeights(phraseDictionary, weightT);
	}

}
