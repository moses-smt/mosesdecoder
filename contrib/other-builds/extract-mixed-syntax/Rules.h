/*
 * Rules.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <set>
#include <iostream>

class AlignedSentence;
class Rule;

class Rules {
public:
	Rules(const AlignedSentence &alignedSentence);
	virtual ~Rules();
	void CreateRules();

	void Debug(std::ostream &out) const;

protected:
	std::set<Rule*> m_todoRules, m_keepRules;

	void Extend(const Rule &rule);

};

