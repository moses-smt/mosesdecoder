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
#include "../../Recycler.h"
#include "../../legacy/Util2.h"

namespace Moses2
{

class Manager;

namespace NSCubePruningBitmapStack
{
typedef Vector<const Hypothesis*>  Hypotheses;

class MiniStack
{
public:
  typedef boost::unordered_set<const Hypothesis*,
          UnorderedComparer<Hypothesis>,
          UnorderedComparer<Hypothesis>
          > _HCType;

  MiniStack(const Manager &mgr);

  StackAdd Add(const Hypothesis *hypo);

  _HCType &GetColl() {
    return m_coll;
  }

  const _HCType &GetColl() const {
    return m_coll;
  }

  void Clear();

  Hypotheses &GetSortedAndPruneHypos(const Manager &mgr) const;

protected:
  _HCType m_coll;
  mutable Hypotheses *m_sortedHypos;

  void SortAndPruneHypos(const Manager &mgr) const;

};

/////////////////////////////////////////////
class Stack
{
protected:


public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
  // bitmap and current endPos of hypos
  typedef boost::unordered_map<HypoCoverage, Hypotheses*> SortedHypos;

  typedef const Bitmap* HypoCoverageInternal;
  typedef boost::unordered_map<HypoCoverageInternal, MiniStack*
  ,boost::hash<HypoCoverageInternal>
  ,std::equal_to<HypoCoverageInternal>
  > Coll;


  Stack(const Manager &mgr);
  virtual ~Stack();

  size_t GetHypoSize() const;

  Coll &GetColl() {
    return m_coll;
  }
  const Coll &GetColl() const {
    return m_coll;
  }

  void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

  MiniStack &GetMiniStack(const HypoCoverageInternal &key);

  std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
  void Clear();

  SortedHypos GetSortedAndPruneHypos(const Manager &mgr) const;
  void SortAndPruneHypos(const Manager &mgr, Hypotheses &hypos) const;

  void DebugCounts();

protected:
  const Manager &m_mgr;
  Coll m_coll;

  std::deque<MiniStack*> m_miniStackRecycler;


};

}

}


