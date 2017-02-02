/*
 * NBestColl.cpp
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "util/exception.hh"
#include "NBestColl.h"
#include "NBests.h"
#include "../Manager.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

/////////////////////////////////////////////////////////////
NBestColl::~NBestColl()
{
  BOOST_FOREACH(const Coll::value_type &valPair, m_candidates) {
    NBests *nbests = valPair.second;
    delete nbests;
  }
}

void NBestColl::Add(const SCFG::Manager &mgr, const ArcList &arcList)
{
  NBests &nbests = GetOrCreateNBests(mgr, arcList);
  //cerr << "nbests for " << &nbests << ":";
}

NBests &NBestColl::GetOrCreateNBests(const SCFG::Manager &mgr, const ArcList &arcList)
{
  NBests *ret;
  Coll::iterator iter = m_candidates.find(&arcList);
  if(iter == m_candidates.end()) {
    ret = new NBests(mgr, arcList, *this);
    m_candidates[&arcList] = ret;
  } else {
    ret = iter->second;
  }
  return *ret;
}


}
}

