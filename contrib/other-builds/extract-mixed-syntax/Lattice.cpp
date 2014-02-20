/*
 * Latticea.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include "Lattice.h"
#include "AlignedSentence.h"

Lattice::Lattice(const AlignedSentence &alignedSentence)
:m_coll(alignedSentence.GetPhrase(Moses::Input).size())
{



}

Lattice::~Lattice() {
	// TODO Auto-generated destructor stub
}

