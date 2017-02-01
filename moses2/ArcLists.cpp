/*
 * ArcList.cpp
 *
 *  Created on: 26 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <boost/foreach.hpp>
#include "ArcLists.h"
#include "HypothesisBase.h"
#include "util/exception.hh"

using namespace std;

namespace Moses2
{

ArcLists::ArcLists()
{
  // TODO Auto-generated constructor stub

}

ArcLists::~ArcLists()
{
  BOOST_FOREACH(const Coll::value_type &collPair, m_coll) {
    const ArcList *arcList = collPair.second;
    delete arcList;
  }
}

void ArcLists::AddArc(bool added, const HypothesisBase *currHypo,
                      const HypothesisBase *otherHypo)
{
  //cerr << added << " " << currHypo << " " << otherHypo << endl;
  ArcList *arcList;
  if (added) {
    // we're winners!
    if (otherHypo) {
      // there was a existing losing hypo
      arcList = &GetAndDetachArcList(otherHypo);
    } else {
      // there was no existing hypo
      arcList = new ArcList;
    }
    m_coll[currHypo] = arcList;
  } else {
    // we're losers!
    // there should be a winner, we're not doing beam pruning
    UTIL_THROW_IF2(otherHypo == NULL, "There must have been a winning hypo");
    arcList = &GetArcList(otherHypo);
  }

  // in any case, add the curr hypo
  arcList->push_back(currHypo);
}

ArcList &ArcLists::GetArcList(const HypothesisBase *hypo)
{
  Coll::iterator iter = m_coll.find(hypo);
  UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list");
  ArcList &arcList = *iter->second;
  return arcList;
}

const ArcList &ArcLists::GetArcList(const HypothesisBase *hypo) const
{
  Coll::const_iterator iter = m_coll.find(hypo);

  if (iter == m_coll.end()) {
    cerr << "looking for:" << hypo << " have " << m_coll.size() << " :";
    BOOST_FOREACH(const Coll::value_type &collPair, m_coll) {
      const HypothesisBase *hypo = collPair.first;
      cerr << hypo << " ";
    }
  }

  UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list for " << hypo);
  ArcList &arcList = *iter->second;
  return arcList;
}

ArcList &ArcLists::GetAndDetachArcList(const HypothesisBase *hypo)
{
  Coll::iterator iter = m_coll.find(hypo);
  UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list");
  ArcList &arcList = *iter->second;

  m_coll.erase(iter);

  return arcList;
}

void ArcLists::Sort()
{
  BOOST_FOREACH(Coll::value_type &collPair, m_coll) {
    ArcList &list = *collPair.second;
    std::sort(list.begin(), list.end(), HypothesisFutureScoreOrderer() );
  }
}

void ArcLists::Delete(const HypothesisBase *hypo)
{
  //cerr << "hypo=" << hypo->Debug() << endl;
  //cerr << "m_coll=" << m_coll.size() << endl;
  Coll::iterator iter = m_coll.find(hypo);
  UTIL_THROW_IF2(iter == m_coll.end(), "Can't find arc list");
  ArcList *arcList = iter->second;

  m_coll.erase(iter);
  delete arcList;
}

std::string ArcLists::Debug(const System &system) const
{
  stringstream strm;
  BOOST_FOREACH(const Coll::value_type &collPair, m_coll) {
    const ArcList *arcList = collPair.second;
    strm << arcList << "(" << arcList->size() << ") ";
  }
  return strm.str();
}

}

