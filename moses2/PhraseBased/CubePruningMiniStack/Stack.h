/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <deque>
#include "../Hypothesis.h"
#include "../../TypeDef.h"
#include "../../Vector.h"
#include "../../MemPool.h"
#include "../../MemPoolAllocator.h"
#include "../../Recycler.h"
#include "../../HypothesisColl.h"
#include "../../legacy/Util2.h"

namespace Moses2
{

class Manager;
class HypothesisBase;
class ArcLists;

namespace NSCubePruningMiniStack
{

class Stack
{
protected:

public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
  // bitmap and current endPos of hypos

  typedef boost::unordered_map<HypoCoverage, Moses2::HypothesisColl*,
          boost::hash<HypoCoverage>, std::equal_to<HypoCoverage>,
          MemPoolAllocator<std::pair<HypoCoverage, Moses2::HypothesisColl*> > > Coll;

  Stack(const Manager &mgr);
  virtual ~Stack();

  size_t GetHypoSize() const;

  Coll &GetColl() {
    return m_coll;
  }
  const Coll &GetColl() const {
    return m_coll;
  }

  void Add(Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
           ArcLists &arcLists);

  Moses2::HypothesisColl &GetMiniStack(const HypoCoverage &key);

  const Hypothesis *GetBestHypo() const;
  void Clear();

  void DebugCounts();

protected:
  const Manager &m_mgr;
  Coll m_coll;

  std::deque<Moses2::HypothesisColl*, MemPoolAllocator<Moses2::HypothesisColl*> > m_miniStackRecycler;

};

}

}

