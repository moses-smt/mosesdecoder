/*
 * Rule.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once
#include <vector>
#include "Phrase.h"

class ConsistentPhrase;
class AlignedSentence;

class Rule {
public:
	Rule(const ConsistentPhrase &consistentPhrase, const AlignedSentence &alignedSentence);
	virtual ~Rule();

	bool IsValid() const
	{ return m_isValid; }

	bool CanRecurse() const
	{ return m_canRecurse; }

protected:
	const ConsistentPhrase &m_consistentPhrase;
	const AlignedSentence &m_alignedSentence;
	Phrase m_source, m_target;

	// in source order
	std::vector<const ConsistentPhrase*> m_nonterms;

	bool m_isValid, m_canRecurse;

	void CreateSource();
};

