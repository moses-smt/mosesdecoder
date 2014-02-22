/*
 * ConsistentPhrases.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */
#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <ConsistentPhrase.h>

class Word;
class NonTerm;

class ConsistentPhrases {
public:
	typedef std::vector<NonTerm*> NonTerms;
	typedef std::map<ConsistentPhrase, NonTerms> Coll;

	ConsistentPhrases();
	virtual ~ConsistentPhrases();

	void Initialize(size_t size);

	void Add(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	const Coll &GetColl(int sourceStart, int sourceEnd) const;

	std::string Debug() const;

protected:
	std::vector< std::vector<Coll> > m_coll;
};

