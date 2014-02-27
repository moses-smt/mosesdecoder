/*
 * ConsistentPhrases.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <set>
#include <vector>
#include <iostream>
#include "ConsistentPhrase.h"

class Word;

class ConsistentPhrases {
public:
	typedef std::set<ConsistentPhrase*> Coll;

	ConsistentPhrases();
	virtual ~ConsistentPhrases();

	void Initialize(size_t size);

	void Add(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	void AddHieroNonTerms();

	const Coll &GetColl(int sourceStart, int sourceEnd) const;
	Coll &GetColl(int sourceStart, int sourceEnd);

	std::string Debug() const;

protected:
	std::vector< std::vector<Coll> > m_coll;
};

