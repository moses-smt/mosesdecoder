/*
 * ConsistentPhrase.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include <ConsistentPhrase.h>
#include <Word.h>

ConsistentPhrase::ConsistentPhrase(
		const Word *sourceStart,
		const Word *sourceEnd,
		const Word *targetStart,
		const Word *targetEnd)
:corners(4)
{
	corners[0] = sourceStart;
	corners[1] = sourceEnd;
	corners[2] = targetStart;
	corners[3] = targetEnd;
}

ConsistentPhrase::~ConsistentPhrase() {
	// TODO Auto-generated destructor stub
}

bool ConsistentPhrase::operator<(const ConsistentPhrase &other) const
{
  return corners < other.corners;
}

void ConsistentPhrase::Debug(std::ostream &out) const
{
  out << "[" << corners[0]->GetPos() << "-" << corners[1]->GetPos()
		  << "][" << corners[2]->GetPos() << "-" << corners[3]->GetPos() << "]";
}
