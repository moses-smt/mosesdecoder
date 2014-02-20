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
	ConsistentPhrases();
	virtual ~ConsistentPhrases();

	size_t GetSize() const
	{ return m_coll.size(); }

	void Add(const Word *sourceStart, const Word *sourceEnd,
			const Word *targetStart, const Word *targetEnd);

	void Debug(std::ostream &out) const;

protected:
	typedef std::set<ConsistentPhrase> Coll;
	Coll m_coll;
};

