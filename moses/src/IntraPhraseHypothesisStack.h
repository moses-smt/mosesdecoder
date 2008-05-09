
#pragma once

#include <set>
#include <vector>
#include "IntraPhraseTargetPhrase.h"

class IntraPhraseHypothesisStack
{
private:
	typedef std::set<IntraPhraseTargetPhrase> CollType;
	CollType m_coll;
	size_t m_maxSize;

public:
	// iter
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }

	IntraPhraseHypothesisStack(size_t maxSize);

	void AddPrune(const IntraPhraseTargetPhrase &phrase);
	void Process();

};

