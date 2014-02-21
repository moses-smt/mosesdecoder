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

class RulePhrase : public std::vector<const RuleSymbol*>
{

};

class Rule {
public:
	Rule(const ConsistentPhrase &consistentPhrase, const AlignedSentence &alignedSentence);
	virtual ~Rule();

	bool IsValid() const
	{ return m_isValid; }

	bool CanRecurse() const
	{ return m_canRecurse; }

	const ConsistentPhrase &GetConsistentPhrase() const
	{ return m_consistentPhrase; }

	int GetNextSourcePosForNonTerm() const;

	void Debug(std::ostream &out) const;

protected:
	const ConsistentPhrase &m_consistentPhrase;
	const AlignedSentence &m_alignedSentence;
	RulePhrase m_source, m_target;

	// in source order
	std::vector<const ConsistentPhrase*> m_nonterms;

	bool m_isValid, m_canRecurse;

	void CreateSource();
};

