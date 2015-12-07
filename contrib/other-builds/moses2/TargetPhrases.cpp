/*
 * TargetPhrases.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "TargetPhrases.h"

using namespace std;

TargetPhrases::TargetPhrases() {
	// TODO Auto-generated constructor stub

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

const TargetPhrases *TargetPhrases::Clone(MemPool &pool) const
{

}


