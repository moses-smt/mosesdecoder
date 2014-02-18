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
	typedef std::vector<const LatticeArc*> Node;

	Lattice(const AlignedSentence &alignedSentence);
	virtual ~Lattice();

	size_t GetSize() const
	{ return m_coll.size(); }

	const Node &GetNode(size_t ind) const
	{ return m_coll[ind]; }

protected:
	// all terms and non-terms, placed in stack according to their starting point
	std::vector<Node> m_coll;

};

