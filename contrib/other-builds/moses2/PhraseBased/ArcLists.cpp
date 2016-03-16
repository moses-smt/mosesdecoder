/*
 * ArcList.cpp
 *
 *  Created on: 26 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include <boost/foreach.hpp>
#include "ArcLists.h"
#include "util/exception.hh"

using namespace std;

namespace Moses2
{

ArcLists::ArcLists() {
	// TODO Auto-generated constructor stub

}

ArcLists::~ArcLists() {
  BOOST_FOREACH(const Coll::value_type &collPair, m_coll) {
	  const ArcList *arcList = collPair.second;
	  delete arcList;
  }
}

void ArcLists::AddArc(bool added, const HypothesisBase *currHypo,const HypothesisBase *otherHypo)
{
	//cerr << added << " " << currHypo << " " << otherHypo << endl;
	if (added) {
		// we're winners!
		ArcList *arcList;
		if (otherHypo) {
			// there was a existing losing hypo
			arcList = GetAndDetachArcList(otherHypo);
			arcList->push_back(otherHypo);
		}
		else {
			// there was no existing hypo
			arcList = new ArcList;
		}
		m_coll[currHypo] = arcList;
	}
	else {
		// we're losers!
		// there should be a winner, we're not doing beam pruning
		UTIL_THROW_IF2(otherHypo == NULL, "There must have been a winning hypo");
		ArcList *arcList = GetArcList(otherHypo);
		arcList->push_back(currHypo);
	}
}

ArcList *ArcLists::GetArcList(const HypothesisBase *hypo)
{
	Coll::iterator iter = m_coll.find(hypo);
	UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list");
	ArcList *arcList = iter->second;
	return arcList;
}

ArcList *ArcLists::GetAndDetachArcList(const HypothesisBase *hypo)
{
	Coll::iterator iter = m_coll.find(hypo);
	UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list");
	ArcList *arcList = iter->second;

	m_coll.erase(iter);

	return arcList;
}

}

