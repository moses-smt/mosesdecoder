
#pragma once

#include <set>
#include <vector>
#include "IntraPhraseTargetPhrase.h"

class IntraPhraseHypothesisStack
{
	typedef std::set<IntraPhraseTargetPhrase*> CollType;
public:
	// iter
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;

private:
	CollType m_coll;
	size_t m_maxSize;
	bool m_isLastStack;

	void PruneToSize(size_t newSize);
	void Remove(const IntraPhraseHypothesisStack::iterator &iter)
	{
		const IntraPhraseTargetPhrase *phrase = *iter;
		m_coll.erase(iter);
		delete phrase;
	}

public:
	// iter
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	iterator begin() { return m_coll.begin(); }
	iterator end() { return m_coll.end(); }

	IntraPhraseHypothesisStack(size_t maxSize, bool isLastStack);
	~IntraPhraseHypothesisStack();

	bool IsLastStack() const
	{ return m_isLastStack; }

	size_t GetSize() const
	{ return m_coll.size(); }

	void AddPrune(IntraPhraseTargetPhrase *phrase);
	void ClearAll()
	{
		RemoveAllInColl(m_coll);
	}
	void RemoveSelectedPhrases();
	
};

