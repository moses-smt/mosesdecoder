/*
 * Stack.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <algorithm>
#include <boost/foreach.hpp>
#include "Stack.h"
#include "../Hypothesis.h"
#include "../Manager.h"
#include "../../Scores.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{

namespace NSCubePruningCardinalStack
{

///////////////////////////////////////////////////////////////
Stack::Stack(const Manager &mgr)
:m_mgr(mgr)
,m_coll(MemPoolAllocator<const Hypothesis*>(mgr.GetPool()))
{
}

Stack::~Stack() {
	// TODO Auto-generated destructor stub
}

void Stack::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
  std::pair<_HCType::iterator, bool> addRet = m_coll.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
	// equiv hypo doesn't exists
  }
  else {
	  const Hypothesis *hypoExisting = *addRet.first;
	  if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
		  // incoming hypo is better than the one we have
		  const Hypothesis *const &hypoExisting1 = *addRet.first;
		  const Hypothesis *&hypoExisting2 = const_cast<const Hypothesis *&>(hypoExisting1);
		  hypoExisting2 = hypo;

		  //hypoExisting->Prefetch();
		  Hypothesis *hypoToBeDeleted = const_cast<Hypothesis*>(hypoExisting);
		  hypoRecycle.Recycle(hypoToBeDeleted);
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  Hypothesis *hypoToBeDeleted = const_cast<Hypothesis*>(hypo);
		  hypoRecycle.Recycle(hypoToBeDeleted);
	  }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret;
  ret.insert(ret.end(), m_coll.begin(), m_coll.end());

  std::vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num)
			   ? ret.end()
			   : ret.begin()+num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
		  HypothesisFutureScoreOrderer());

  return ret;
}

size_t Stack::GetHypoSize() const
{
	return m_coll.size();
}

void Stack::Clear()
{

	m_coll.clear();
}

Stack::SortedHypos Stack::GetSortedAndPruneHypos(const Manager &mgr) const
{
  SortedHypos ret;
/*
  // divide hypos by [bitmap, last end pos]
  BOOST_FOREACH(const Hypothesis *hypo, m_coll) {
	  HypoCoverage key(&hypo->GetBitmap(), hypo->GetInputPath().range.GetEndPos());

	  Hypotheses *hypos;
	  SortedHypos::const_iterator iter;
	  iter = ret.find(key);
	  if (iter == ret.end()) {
		  hypos = new (pool.Allocate< Vector<const Hypothesis*> >()) Vector<const Hypothesis*>(pool, m_coll.size());
	  }
	  ret[key]->push_back(hypo);
  }
*/
  if (m_sortedHypos == NULL) {
    // create sortedHypos first
    MemPool &pool = mgr.GetPool();
	m_sortedHypos = new (pool.Allocate< Vector<const Hypothesis*> >()) Vector<const Hypothesis*>(pool, m_coll.size());

	  size_t ind = 0;
	  BOOST_FOREACH(const Hypothesis *hypo, m_coll) {
		  (*m_sortedHypos)[ind] = hypo;
		  ++ind;
	  }

    SortAndPruneHypos(mgr);
  }

  return ret;
}

void Stack::SortAndPruneHypos(const Manager &mgr) const
{
  size_t stackSize = mgr.system.stackSize;
  Recycler<Hypothesis*> &recycler = mgr.GetHypoRecycle();

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
		  Hypothesis *hypo = const_cast<Hypothesis*>((*m_sortedHypos)[i]);
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


}

}

