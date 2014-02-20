/*
 * ConsistentPhrase.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <iostream>

class ConsistentPhrase {
public:
	ConsistentPhrase(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	virtual ~ConsistentPhrase();

	std::vector<int> corners;

  bool operator<(const ConsistentPhrase &other) const;

	void Debug(std::ostream &out) const;

};

