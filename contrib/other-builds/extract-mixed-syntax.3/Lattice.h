/*
 * Latticea.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <map>
#include <vector>
#include <cstddef>
#include "LatticeArc.h"

class AlignedSentence;
class Word;

class Lattice {
public:
	// list of arcs coming from a particular node
	typedef std::vector<const LatticeArc*> Node;

	// 1st = start source node
	// list of arcs coming from that node
	typedef std::map<const Word*, Node> Coll;

	Lattice(const AlignedSentence &alignedSentence);
	virtual ~Lattice();

	size_t GetSize() const
	{ return m_coll.size(); }

	//const Node &GetNode(size_t ind) const
	//{ return m_coll[ind]; }

protected:
	// all terms and non-terms, placed in stack according to their starting point
	Coll m_coll;

};

