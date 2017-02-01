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

namespace NSCubePruningCardinalStack
{
typedef Vector<const Hypothesis*>  Hypotheses;


/////////////////////////////////////////////
class Stack
{
protected:
  typedef boost::unordered_set<const Hypothesis*,
          UnorderedComparer<Hypothesis>,
          UnorderedComparer<Hypothesis>
          > _HCType;

public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
  typedef boost::unordered_map<HypoCoverage, Hypotheses*> SortedHypos;

  Stack(const Manager &mgr);
  virtual ~Stack();

  size_t GetHypoSize() const;

  _HCType &GetColl() {
    return m_coll;
  }
  const _HCType &GetColl() const {
    return m_coll;
  }

  void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

  std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
  void Clear();

  SortedHypos GetSortedAndPruneHypos(const Manager &mgr) const;
  void SortAndPruneHypos(const Manager &mgr, Hypotheses &hypos) const;

protected:
  const Manager &m_mgr;
  _HCType m_coll;

};

}

}


