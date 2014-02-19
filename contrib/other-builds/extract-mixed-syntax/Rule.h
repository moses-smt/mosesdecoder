/*
 * Rule.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <vector>

class LatticeArc;
class Parameter;
class ConsistentPhrase;
class ConsistentPhrases;

class Rule {
public:
	Rule(const LatticeArc &arc);
	Rule(const Rule &prevRule, const LatticeArc &arc);

	virtual ~Rule();

	bool IsValid(const Parameter &params) const;
	bool CanExtend(const Parameter &params) const;
	void Fillout(const ConsistentPhrases &consistentPhrases);

	const LatticeArc &GetLastArc() const
	{ return *m_arcs.back(); }

	Rule *Extend(const LatticeArc &arc) const;

protected:
	std::vector<const LatticeArc*> m_arcs;
	std::vector<const LatticeArc*> m_targetArcs;

	const ConsistentPhrase *m_consistentPhrase;
};

