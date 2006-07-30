
#pragma once

#include <map>
#include <vector>
#include <iterator>
#include "Word.h"
#include "TargetPhraseCollection.h"

class PhraseDictionary;

class PhraseDictionaryNode
{
	typedef std::map<Word, PhraseDictionaryNode> NodeMap;
protected:
	NodeMap m_map;
	TargetPhraseCollection *m_targetPhraseCollection;

public:
	PhraseDictionaryNode()
		:m_targetPhraseCollection(NULL)
	{}
	~PhraseDictionaryNode()
	{
		delete m_targetPhraseCollection;
	}

	PhraseDictionaryNode *GetOrCreateChild(const Word word);
	const PhraseDictionaryNode *GetChild(const Word word) const;
	const TargetPhraseCollection *GetTargetPhraseCollection() const
	{
		return m_targetPhraseCollection;
	}
	TargetPhraseCollection *CreateTargetPhraseCollection()
	{
		if (m_targetPhraseCollection == NULL)
			m_targetPhraseCollection = new TargetPhraseCollection();
		return m_targetPhraseCollection;
	}
	// for mert
	void SetWeightTransModel(const PhraseDictionary *phraseDictionary
													, const std::vector<float> &weightT);

	// iterators
	typedef NodeMap::iterator iterator;
	typedef NodeMap::const_iterator const_iterator;
	const_iterator begin() const { return m_map.begin(); }
	const_iterator end() const { return m_map.end(); }
	iterator begin() { return m_map.begin(); }
	iterator end() { return m_map.end(); }
};
