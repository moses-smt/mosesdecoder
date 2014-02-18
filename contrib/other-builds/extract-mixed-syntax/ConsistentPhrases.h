/*
 * ConsistentPhrases.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <vector>
#include "ConsistentPhrase.h"

class ConsistentPhrases {

  typedef std::vector<ConsistentPhrase> Coll;

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

  void Add(ConsistentPhrase &phrasePair);

protected:
  Coll m_coll;

};

