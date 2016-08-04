/*
 * HypothesisColl.h
 *
 *  Created on: 26 Feb 2016
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_set.hpp>
#include "HypothesisBase.h"
#include "MemPool.h"
#include "Recycler.h"
#include "Array.h"
#include "legacy/Util2.h"

namespace Moses2
{

class ManagerBase;
class ArcLists;

typedef Array<const HypothesisBase*> Hypotheses;

class HypothesisColl
{
public:
  typedef boost::unordered_set<const HypothesisBase*,
      UnorderedComparer<HypothesisBase>, UnorderedComparer<HypothesisBase>,
      MemPoolAllocator<const HypothesisBase*> > _HCType;

  typedef _HCType::iterator iterator;
  typedef _HCType::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const
  {
    return m_coll.begin();
  }
  const_iterator end() const
  {
    return m_coll.end();
  }

  HypothesisColl(const ManagerBase &mgr);

  void Add(const System &system,
		  HypothesisBase *hypo,
		  Recycler<HypothesisBase*> &hypoRecycle,
		  ArcLists &arcLists);

  _HCType &GetColl()
  {
    return m_coll;
  }

  const _HCType &GetColl() const
  {
    return m_coll;
  }

  size_t GetSize() const
  {
    return m_coll.size();
  }

  void Clear();

  const Hypotheses &GetSortedAndPruneHypos(
      const ManagerBase &mgr,
      ArcLists &arcLists) const;

  std::string Debug(const System &system) const;

protected:
  _HCType m_coll;
  mutable Hypotheses *m_sortedHypos;

  StackAdd Add(const HypothesisBase *hypo);
  void SortAndPruneHypos(const ManagerBase &mgr, ArcLists &arcLists) const;

};

} /* namespace Moses2 */

