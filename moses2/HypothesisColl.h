/*
 * HypothesisColl.h
 *
 *  Created on: 26 Feb 2016
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_set.hpp>
#include "HypothesisBase.h"
#include "MemPoolAllocator.h"
#include "Recycler.h"
#include "Array.h"
#include "legacy/Util2.h"

namespace Moses2
{

class ManagerBase;
class ArcLists;

typedef Array<const HypothesisBase*> Hypotheses;

////////////////////////////////////////////////////
class HypothesisColl
{
public:
  HypothesisColl(const ManagerBase &mgr);

  void Add(const ManagerBase &mgr,
           HypothesisBase *hypo,
           Recycler<HypothesisBase*> &hypoRecycle,
           ArcLists &arcLists);

  size_t GetSize() const {
    return m_coll.size();
  }

  void Clear();

  const Hypotheses &GetSortedAndPrunedHypos(
    const ManagerBase &mgr,
    ArcLists &arcLists) const;

  const HypothesisBase *GetBestHypo() const;

  template<typename T>
  const T *GetBestHypo() const {
    const HypothesisBase *hypo = GetBestHypo();
    return hypo ? &hypo->Cast<T>() : NULL;
  }

  void Delete(const HypothesisBase *hypo);

  std::string Debug(const System &system) const;

protected:
  typedef boost::unordered_set<const HypothesisBase*,
		  UnorderedComparer<HypothesisBase>, UnorderedComparer<HypothesisBase>,
          MemPoolAllocator<const HypothesisBase*> > _HCType;

  _HCType m_coll;
  mutable Hypotheses *m_sortedHypos;

  SCORE m_bestScore;
  SCORE m_worstScore;

  StackAdd Add(const HypothesisBase *hypo);

  void PruneHypos(const ManagerBase &mgr, ArcLists &arcLists);
  void SortHypos(const ManagerBase &mgr, const HypothesisBase **sortedHypos) const;

};

} /* namespace Moses2 */

