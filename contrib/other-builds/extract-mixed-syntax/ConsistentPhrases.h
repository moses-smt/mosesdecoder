/*
 * ConsistentPhrases.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <set>
#include <iostream>
#include <ConsistentPhrase.h>

class Word;

class ConsistentPhrases {
public:
	typedef std::set<ConsistentPhrase> Coll;

	ConsistentPhrases();
	virtual ~ConsistentPhrases();

	void Initialize(size_t size);

	void Add(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	const Coll &GetColl(int sourceStart, int sourceEnd) const;

	void Debug(std::ostream &out) const;

protected:
	std::vector< std::vector<Coll> > m_coll;
};

