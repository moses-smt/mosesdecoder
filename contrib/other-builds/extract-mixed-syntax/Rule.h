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
	Rule(const LatticeArc *arc);
	virtual ~Rule();

	bool IsValid() const;
	void Fillout();

protected:
	std::vector<const LatticeArc*> m_arcs;
};

