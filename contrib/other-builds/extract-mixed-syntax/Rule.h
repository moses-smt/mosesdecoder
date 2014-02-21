/*
 * Rule.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once

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
	Phrase m_source, m_target;
	bool m_isValid, m_canRecurse;
};

