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

class AlignedSentence;
class Rule;

class Rules {
public:
	Rules(const AlignedSentence &alignedSentence);
	virtual ~Rules();
	void CreateRules();

	std::string Debug() const;
	void Output(std::ostream &out) const;

protected:
	const AlignedSentence &m_alignedSentence;
	std::set<Rule*> m_todoRules, m_keepRules;

	void Extend(const Rule &rule);
	void Extend(const Rule &rule, const ConsistentPhrases::Coll &cps);
	void Extend(const Rule &rule, const ConsistentPhrase &cp);

};

