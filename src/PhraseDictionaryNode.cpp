
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
