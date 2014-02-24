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
class NonTerm;
class Parameter;

class RulePhrase : public std::vector<const RuleSymbol*>
{

};

class Rule {
public:
	Rule(const Rule &copy); // do not implement

	// original rule with no non-term
	Rule(const NonTerm &lhsNonTerm, const AlignedSentence &alignedSentence);

	// extend a rule, adding 1 new non-term
	Rule(const Rule &copy, const NonTerm &nonTerm);

	virtual ~Rule();

	bool IsValid() const
	{ return m_isValid; }

	bool CanRecurse() const
	{ return m_canRecurse; }

	const NonTerm &GetLHS() const
	{ return m_lhs; }

	const ConsistentPhrase &GetConsistentPhrase() const;

	int GetNextSourcePosForNonTerm() const;

	std::string Debug() const;
	void Output(std::ostream &out) const;

	void Prevalidate(const Parameter &params);
	void CreateTargetPhrase(const AlignedSentence &alignedSentence,
					const Parameter &params);

protected:
	const NonTerm &m_lhs;
	const AlignedSentence &m_alignedSentence;
	RulePhrase m_source, m_target;

	// in source order
	std::vector<const NonTerm*> m_nonterms;

	bool m_isValid, m_canRecurse;

	void CreateSource();
};

