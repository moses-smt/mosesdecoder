
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

	void PruneToSize(size_t newSize);
	void Remove(const IntraPhraseHypothesisStack::iterator &iter)
	{
		m_coll.erase(iter);
	}
public:
	// iter
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }

	IntraPhraseHypothesisStack(size_t maxSize);
	~IntraPhraseHypothesisStack();

	size_t GetSize() const
	{ return m_coll.size(); }

	void AddPrune(IntraPhraseTargetPhrase *phrase);

};

