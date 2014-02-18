/*
 * Rule.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <vector>

class LatticeArc;

class Rule {
public:
	Rule(const LatticeArc &arc);
	virtual ~Rule();

	bool IsValid() const;
	bool CanExtend() const;
	void Fillout();

	const LatticeArc &GetLastArc() const
	{ return *m_arcs.back(); }

	Rule *Extend(const LatticeArc &arc) const;

protected:
	std::vector<const LatticeArc*> m_arcs;
};

