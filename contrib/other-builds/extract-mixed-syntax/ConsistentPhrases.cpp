/*
 * ConsistentPhrases.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <cassert>
#include "ConsistentPhrases.h"

using namespace std;

ConsistentPhrases::ConsistentPhrases() {
	// TODO Auto-generated constructor stub

}

ConsistentPhrases::~ConsistentPhrases() {
	// TODO Auto-generated destructor stub
}

void ConsistentPhrases::Add(ConsistentPhrase &phrasePair)
{
	m_coll.push_back(phrasePair);

	int sourceStart = phrasePair.GetConsistentRange(Moses::Input).GetStart();
	int sourceEnd = phrasePair.GetConsistentRange(Moses::Input).GetEnd();
	int targetStart = phrasePair.GetConsistentRange(Moses::Output).GetStart();
	int targetEnd = phrasePair.GetConsistentRange(Moses::Output).GetEnd();

	PhrasePairRange phrasePairRange(sourceStart, sourceEnd, targetStart, targetEnd);
	m_rangeToColl[phrasePairRange] = &m_coll.back();
}

const ConsistentPhrase *ConsistentPhrases::Find(
		int sourceStart,
		int sourceEnd,
		int targetStart,
		int targetEnd) const
{
	PhrasePairRange phrasePairRange(sourceStart, sourceEnd, targetStart, targetEnd);

	RangeToColl::const_iterator iter = m_rangeToColl.find(phrasePairRange);
	if (iter == m_rangeToColl.end()) {
		return NULL;
	}
	else {
		return iter->second;
	}
}

void ConsistentPhrases::Debug(std::ostream &out) const
{
  out << "m_rangeToColl=" << m_rangeToColl.size()
	  << " m_coll=" << m_coll.size() <<endl;
  for (size_t i = 0; i < m_coll.size(); ++i) {
	  const ConsistentPhrase &consistentPhrase = m_coll[i];
	  consistentPhrase.Debug(out);
	  out << endl;
  }
}
