/*
 * Rules.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <set>
#include <iostream>
#include "ConsistentPhrases.h"
#include "Rule.h"

class AlignedSentence;
class Parameter;

struct CompareRules {
	bool operator()(const Rule *a, const Rule *b)
	{

	}
};

class Rules {
public:
	Rules(const AlignedSentence &alignedSentence);
	virtual ~Rules();
	void Extend(const Parameter &params);
	void Consolidate(const Parameter &params);

	std::string Debug() const;
	void Output(std::ostream &out) const;

protected:
	const AlignedSentence &m_alignedSentence;
	std::set<Rule*> m_keepRules;
	std::set<Rule*, CompareRules> m_mergeRules;

	void Extend(const Rule &rule, const Parameter &params);
	void Extend(const Rule &rule, const ConsistentPhrases::Coll &cps, const Parameter &params);
	void Extend(const Rule &rule, const ConsistentPhrase &cp, const Parameter &params);

	// create original rules
	void CreateRules(const ConsistentPhrase &cp,
			const Parameter &params);

	void MergeRules(const Parameter &params);

};

