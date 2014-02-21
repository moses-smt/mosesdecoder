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
	typedef std::set<ConsistentPhrase> Coll;
public:
	  typedef Coll::iterator iterator;
	  typedef Coll::const_iterator const_iterator;
	  //! iterators
	  const_iterator begin() const {
		return m_coll.begin();
	  }
	  const_iterator end() const {
		return m_coll.end();
	  }

	ConsistentPhrases();
	virtual ~ConsistentPhrases();

	size_t GetSize() const
	{ return m_coll.size(); }

	void Add(int sourceStart, int sourceEnd,
			int targetStart, int targetEnd);

	void Debug(std::ostream &out) const;

protected:
	Coll m_coll;


	std::vector< std::vector<const ConsistentPhrase*> > m_bySourceRange;
};

