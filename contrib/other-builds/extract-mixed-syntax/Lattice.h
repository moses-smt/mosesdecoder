/*
 * Lattice.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <vector>
#include "LatticeArc.h"

class AlignedSentence;

class Lattice
{
public:
	typedef std::vector<const LatticeArc*> LatticeNode;

	Lattice(const AlignedSentence &alignedSentence);
	virtual ~Lattice();

protected:
	// all terms and non-terms, placed in stack according to their starting point
	std::vector<LatticeNode> m_coll;

};

