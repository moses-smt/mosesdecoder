
#pragma once

#include <list>
#include <vector>
#include "TargetPhrase.h"

namespace Moses
{
	class TargetPhraseCollection;
};

namespace MosesOnDiskPt
{

class TargetPhraseList
{
protected:
	typedef std::list<const Phrase*> PhraseCollType;
	PhraseCollType m_phraseCache; // store source phrases referenced by target phrases

	typedef std::vector<const TargetPhrase*> TargetPhraseCollType;
	TargetPhraseCollType m_coll;

public:
	typedef TargetPhraseCollType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }

	virtual ~TargetPhraseList();

	size_t GetSize() const
	{ return m_coll.size(); }

	void Add(const Phrase *phrase, const TargetPhrase *targetPhrase)
	{
		m_phraseCache.push_back(phrase);
		m_coll.push_back(targetPhrase);
	}

};

};

