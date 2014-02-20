/*
 * ConsistentPhrase.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <iostream>

class Word;

class ConsistentPhrase {
public:
	ConsistentPhrase(const Word *sourceStart,
					const Word *sourceEnd,
					const Word *targetStart,
					const Word *targetEnd);

	virtual ~ConsistentPhrase();

	std::vector<const Word *> corners;

  bool operator<(const ConsistentPhrase &other) const;

	void Debug(std::ostream &out) const;

};

