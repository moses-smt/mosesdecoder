/*
 * Vocab.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Vocab.h"

Vocab::Vocab() {
	// TODO Auto-generated constructor stub

}

Vocab::~Vocab() {
	// TODO Auto-generated destructor stub
}

const Factor *Vocab::AddFactor(const StringPiece &string)
{
	Factor in(string);
	std::pair<Set::iterator, bool> ret = m_set.insert(in);
	const Factor &out = *ret.first;
	return &out;
}

