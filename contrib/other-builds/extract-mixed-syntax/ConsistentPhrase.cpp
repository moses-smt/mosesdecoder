/*
 * ConsistentPhrase.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#include <sstream>
#include "ConsistentPhrase.h"
#include "Word.h"

using namespace std;

ConsistentPhrase::ConsistentPhrase(
		int sourceStart, int sourceEnd,
		int targetStart, int targetEnd)
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

std::string ConsistentPhrase::Debug() const
{
  stringstream out;
  out << "[" << corners[0] << "-" << corners[1]
		  << "][" << corners[2] << "-" << corners[3] << "]";

  return out.str();
}

void ConsistentPhrase::Output(std::ostream &out) const
{
  out << "[X][X]";
}

void ConsistentPhrase::Output(std::ostream &out, Moses::FactorDirection direction) const
{
  out << "[X]";
}
