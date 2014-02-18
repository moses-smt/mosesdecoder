/*
 * Rules.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <set>

class Lattice;
class AlignedSentence;
class Rule;
class Parameter;

class Rules {
public:
	Rules(const Lattice &lattice, const AlignedSentence &alignedSentence);
	virtual ~Rules();

	void CreateRules(const Parameter &params);


protected:
	const Lattice &m_lattice;
	const AlignedSentence &m_alignedSentence;

	std::set<Rule*> m_activeRules, m_keepRules;;

	void Extend(const Rule &rule, const Parameter &params);
};

