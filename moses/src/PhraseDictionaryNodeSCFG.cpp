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

#include "PhraseDictionaryNodeSCFG.h"
#include "TargetPhrase.h"
#include "PhraseDictionaryMemory.h"

namespace Moses
{
	
PhraseDictionaryNodeSCFG::~PhraseDictionaryNodeSCFG()
{
	delete m_targetPhraseCollection;
}

void PhraseDictionaryNodeSCFG::Sort(size_t tableLimit)
{
	// recusively sort
	NodeMap::iterator iter;
	for (iter = m_map.begin() ; iter != m_map.end() ; ++iter)
	{
		InnerNodeMap &innerMap = iter->second;
		InnerNodeMap::iterator iterInner;
		for (iterInner = innerMap.begin() ; iterInner != innerMap.end() ; ++iterInner)
		{
			iterInner->second.Sort(tableLimit);
		}
	}
	
	// sort TargetPhraseCollection in this node
	if (m_targetPhraseCollection != NULL)
		m_targetPhraseCollection->NthElement(tableLimit);
}

PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetOrCreateChild(const Word &word, const Word &sourcelabel)
{
	InnerNodeMap *innerNodeMap;
	innerNodeMap = &m_map[sourcelabel];

	InnerNodeMap::iterator iter = innerNodeMap->find(word);
	if (iter != innerNodeMap->end())
		return &iter->second;	// found it

	// can't find node. create a new 1
	std::pair <InnerNodeMap::iterator,bool> insResult; 
	insResult = innerNodeMap->insert( std::make_pair(word, PhraseDictionaryNodeSCFG()) );
	assert(insResult.second);

	iter = insResult.first;
	PhraseDictionaryNodeSCFG &ret = iter->second;
	return &ret;
}
	
const PhraseDictionaryNodeSCFG *PhraseDictionaryNodeSCFG::GetChild(const Word &word, const Word &sourcelabel) const
{
	NodeMap::const_iterator iterOuter = m_map.find(sourcelabel);
	if (iterOuter == m_map.end())
		return NULL;

	const InnerNodeMap &innerNodeMap = iterOuter->second;
	InnerNodeMap::const_iterator iter = innerNodeMap.find(word);
	if (iter != innerNodeMap.end())
		return &iter->second;	// found it

	// don't return anything
	return NULL;
}


void PhraseDictionaryNodeSCFG::SetWeightTransModel(const PhraseDictionary *phraseDictionary
																							 , const std::vector<float> &weightT)
{
	// recursively set weights
	NodeMap::iterator iterOuter;
	for (iterOuter = m_map.begin() ; iterOuter != m_map.end() ; ++iterOuter)
	{
		InnerNodeMap &inner = iterOuter->second;
		InnerNodeMap::iterator iterInner;
		for (iterInner = inner.begin() ; iterInner != inner.end() ; ++iterInner)
		{
			iterInner->second.SetWeightTransModel(phraseDictionary, weightT);
		}
	}

	// set wieghts for this target phrase
	if (m_targetPhraseCollection == NULL)
		return;

	TargetPhraseCollection::iterator iterTargetPhrase;
	for (iterTargetPhrase = m_targetPhraseCollection->begin();
				iterTargetPhrase != m_targetPhraseCollection->end();
				++iterTargetPhrase)
	{
		TargetPhrase &targetPhrase = **iterTargetPhrase;
		targetPhrase.SetWeights(phraseDictionary->GetFeature(), weightT);
	}

}

std::ostream& operator<<(std::ostream &out, const PhraseDictionaryNodeSCFG &node)
{
	out << node.GetTargetPhraseCollection();
	return out;
}

TO_STRING_BODY(PhraseDictionaryNodeSCFG)
	
}

