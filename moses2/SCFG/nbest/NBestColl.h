/*
 * NBestColl.h
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include "../../ArcLists.h"


namespace Moses2
{
namespace SCFG
{
class NBests;
class Manager;

class NBestColl
{
public:
  virtual ~NBestColl();

  void Add(const SCFG::Manager &mgr, const ArcList &arcList);
  NBests &GetOrCreateNBests(const SCFG::Manager &mgr, const ArcList &arcList);

protected:
  typedef boost::unordered_map<const ArcList*, NBests*> Coll;
  Coll m_candidates;

};

}
}


