/*
 * HypothesisColl.cpp
 *
 *  Created on: 26 Feb 2016
 *      Author: hieu
 */
#include <algorithm>
#include <boost/foreach.hpp>
#include "HypothesisColl.h"
#include "ManagerBase.h"
#include "System.h"

namespace Moses2 {

HypothesisColl::HypothesisColl(const ManagerBase &mgr)
:m_coll(MemPoolAllocator<const HypothesisBase*>(mgr.GetPool()))
,m_sortedHypos(NULL)
{}

StackAdd HypothesisColl::Add(const HypothesisBase *hypo)
{
  std::pair<_HCType::iterator, bool> addRet = m_coll.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
	// equiv hypo doesn't exists
	return StackAdd(true, NULL);
  }
  else {
	  const HypothesisBase *hypoExisting = *addRet.first;
	  if (hypo->GetFutureScore() > hypoExisting->GetFutureScore()) {
		  // incoming hypo is better than the one we have
		  const HypothesisBase *const &hypoExisting1 = *addRet.first;
		  const HypothesisBase *&hypoExisting2 = const_cast<const HypothesisBase *&>(hypoExisting1);
		  hypoExisting2 = hypo;

		  return StackAdd(true, const_cast<HypothesisBase*>(hypoExisting));
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, const_cast<HypothesisBase*>(hypo));
	  }
  }

  assert(false);
}

Hypotheses &HypothesisColl::GetSortedAndPruneHypos(const ManagerBase &mgr) const
{
  if (m_sortedHypos == NULL) {
    // create sortedHypos first
    MemPool &pool = mgr.GetPool();
	m_sortedHypos = new (pool.Allocate<Hypotheses>()) Hypotheses(pool, m_coll.size());

	  size_t ind = 0;
	  BOOST_FOREACH(const HypothesisBase *hypo, m_coll) {
		  (*m_sortedHypos)[ind] = hypo;
		  ++ind;
	  }

    SortAndPruneHypos(mgr);
  }

  return *m_sortedHypos;
}

void HypothesisColl::SortAndPruneHypos(const ManagerBase &mgr) const
{
  size_t stackSize = mgr.system.stackSize;
  Recycler<HypothesisBase*> &recycler = mgr.GetHypoRecycle();

  /*
  cerr << "UNSORTED hypos:" << endl;
  for (size_t i = 0; i < hypos.size(); ++i) {
	  const Hypothesis *hypo = hypos[i];
	  cerr << *hypo << endl;
  }
  cerr << endl;
  */
  Hypotheses::iterator iterMiddle;
  iterMiddle = (stackSize == 0 || m_sortedHypos->size() < stackSize)
			   ? m_sortedHypos->end()
			   : m_sortedHypos->begin() + stackSize;

  std::partial_sort(m_sortedHypos->begin(), iterMiddle, m_sortedHypos->end(),
		  HypothesisFutureScoreOrderer());

  // prune
  if (stackSize && m_sortedHypos->size() > stackSize) {
	  for (size_t i = stackSize; i < m_sortedHypos->size(); ++i) {
		  HypothesisBase *hypo = const_cast<HypothesisBase*>((*m_sortedHypos)[i]);
		  recycler.Recycle(hypo);
	  }
	  m_sortedHypos->resize(stackSize);
  }

  /*
  cerr << "sorted hypos:" << endl;
  for (size_t i = 0; i < hypos.size(); ++i) {
	  const Hypothesis *hypo = hypos[i];
	  cerr << hypo << " " << *hypo << endl;
  }
  cerr << endl;
  */

}

void HypothesisColl::Clear()
{
	m_sortedHypos = NULL;
	m_coll.clear();
}

std::vector<const HypothesisBase*> HypothesisColl::GetBestHyposAndPrune(size_t num, Recycler<HypothesisBase*> &recycler) const
{
  std::vector<const HypothesisBase*> ret = GetBestHypos(num);
  if (num && ret.size() > num) {
         for (size_t i = num; i < ret.size(); ++i) {
        	 HypothesisBase *hypo = const_cast<HypothesisBase*>(ret[i]);
             recycler.Recycle(hypo);
         }
         ret.resize(num);
  }
  return ret;
}

std::vector<const HypothesisBase*> HypothesisColl::GetBestHypos(size_t num) const
{
  std::vector<const HypothesisBase*> ret(m_coll.begin(), m_coll.end());

  std::vector<const HypothesisBase*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num)
                          ? ret.end()
                          : ret.begin()+num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
                 HypothesisFutureScoreOrderer());

  return ret;
}



} /* namespace Moses2 */
