/*
 * ConsistentPhrases.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <vector>
#include <map>
#include "ConsistentPhrase.h"

///////////////////////////////////////////////////////////////////

// just the range, for finding
struct PhrasePairRange
{
  typedef std::pair<int, int> Range;
  std::pair<Range, Range> sourceTarget;

  PhrasePairRange(int sourceStart, int sourceEnd, int targetStart, int targetEnd)
  {
    sourceTarget.first.first = sourceStart;
	sourceTarget.first.second = sourceEnd;

	sourceTarget.second.first = targetStart;
	sourceTarget.second.second = targetEnd;

  }

  inline bool operator<(const PhrasePairRange &other) const {
	return sourceTarget < other.sourceTarget;
  }

};

///////////////////////////////////////////////////////////////////
class ConsistentPhrases {

  typedef std::vector<ConsistentPhrase> Coll;
  typedef std::map<PhrasePairRange, const ConsistentPhrase*> RangeToColl;

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
  const ConsistentPhrase *Find(int sourceStart, int sourceEnd, int targetStart, int targetEnd) const;

  void Debug(std::ostream &out) const;

protected:
  Coll m_coll;
  RangeToColl m_rangeToColl;

};

