/*
 * TargetPhrases.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include "TargetPhrase.h"

class TargetPhrases {
	friend std::ostream& operator<<(std::ostream &, const TargetPhrases &);
	typedef std::vector<const TargetPhrase*> Coll;
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

	TargetPhrases();
	virtual ~TargetPhrases();

	void AddTargetPhrase(const TargetPhrase &targetPhrase)
	{
		m_coll.push_back(&targetPhrase);
	}
protected:
	Coll m_coll;

};

