/*
 * Rules.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <set>
#include <iostream>

class Lattice;
class AlignedSentence;
class Rule;
class Parameter;
class ConsistentPhrases;

class Rules {
public:
	Rules(const Lattice &lattice, const AlignedSentence &alignedSentence);
	virtual ~Rules();

	void CreateRules(const Parameter &params, const ConsistentPhrases &consistentPhrases);
	void Output(std::ostream &out) const;

protected:
	const Lattice &m_lattice;
	const AlignedSentence &m_alignedSentence;

	std::set<Rule*> m_activeRules, m_keepRules;

	void Extend(const Rule &rule, const Parameter &params);
};

