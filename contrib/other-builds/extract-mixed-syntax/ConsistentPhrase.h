/*
 * ConsistentPhrase.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <iostream>
#include "moses/TypeDef.h"

class ConsistentPhrase {
public:
	ConsistentPhrase(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	virtual ~ConsistentPhrase();

	int GetWidth(Moses::FactorDirection direction) const
	{ return (direction == Moses::Input) ? corners[1] - corners[0] + 1 : corners[3] - corners[2] + 1; }

	std::vector<int> corners;

  bool operator<(const ConsistentPhrase &other) const;

	void Debug(std::ostream &out) const;

};

