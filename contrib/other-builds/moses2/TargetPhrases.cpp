/*
 * TargetPhrases.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <cassert>
#include <boost/foreach.hpp>
#include "TargetPhrases.h"

using namespace std;

TargetPhrases::TargetPhrases(MemPool &pool, size_t reserve)
:m_coll(pool, reserve)
,m_currInd(0)
{
}

TargetPhrases::TargetPhrases(MemPool &pool, const System &system, const TargetPhrases &copy)
:m_coll(pool, copy.m_coll.size())
{
	for (size_t i = 0; i < copy.m_coll.size(); ++i) {
		const TargetPhrase *tpOrig = copy.m_coll[i];
		assert(tpOrig);
		const TargetPhrase *tpClone = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, *tpOrig);
		m_coll[i] = tpClone;
	}
}


TargetPhrases::~TargetPhrases() {
	// TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream &out, const TargetPhrases &obj)
{
	BOOST_FOREACH(const TargetPhrase *tp, obj) {
		out << *tp << endl;
	}

	return out;
}

void TargetPhrases::SortAndPrune(size_t tableLimit)
{
  iterator iterMiddle;
  iterMiddle = (tableLimit == 0 || m_coll.size() < tableLimit)
               ? m_coll.end()
               : m_coll.begin()+tableLimit;

  std::partial_sort(m_coll.begin(), iterMiddle, m_coll.end(),
		  CompareFutureScore());

  if (tableLimit && m_coll.size() > tableLimit) {
	  m_coll.resize(tableLimit);
  }

  //cerr << "TargetPhrases=" << GetSize() << endl;
}

const TargetPhrases *TargetPhrases::Clone(MemPool &pool, const System &system) const
{
	const TargetPhrases *ret = new TargetPhrases(pool, system, *this);
	return ret;
}


