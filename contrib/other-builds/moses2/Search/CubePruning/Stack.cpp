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

namespace NSCubePruning
{
CubeEdge::Hypotheses &HypothesisSet::GetSortedHypos(const Manager &mgr) const
{
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

  return *m_sortedHypos;
}

void HypothesisSet::SortAndPruneHypos(const Manager &mgr) const
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
  Vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (stackSize == 0 || m_sortedHypos->size() < stackSize)
			   ? m_sortedHypos->end()
			   : m_sortedHypos->begin() + stackSize;

  std::partial_sort(m_sortedHypos->begin(), iterMiddle, m_sortedHypos->end(),
		  HypothesisFutureScoreOrderer());

  // prune
  if (stackSize && m_sortedHypos->size() > stackSize) {
	  for (size_t i = stackSize; i < m_sortedHypos->size(); ++i) {
		  Hypothesis *hypo = const_cast<Hypothesis*>((*m_sortedHypos)[i]);
		  recycler.Add(hypo);
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

///////////////////////////////////////////////////////////////
Stack::Stack() {
	// TODO Auto-generated constructor stub

}

Stack::~Stack() {
	// TODO Auto-generated destructor stub
}

void Stack::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
	StackAdd added = Add(hypo);

	if (added.toBeDeleted) {
		hypoRecycle.Add(added.toBeDeleted);
	}

}

StackAdd Stack::Add(const Hypothesis *hypo)
{
  HypoCoverage key(&hypo->GetBitmap(), hypo->GetRange().GetEndPos());
  HypothesisSet::_HCType &innerColl = GetHypothesisSet(key).GetColl();
  std::pair<HypothesisSet::_HCType::iterator, bool> addRet = innerColl.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
    // equiv hypo doesn't exists
	return StackAdd(true, NULL);
  }
  else {
	  const Hypothesis *hypoExisting = *addRet.first;
	  if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
		  // incoming hypo is better than the one we have
		  innerColl.erase(addRet.first);

		  // re-add. It better go in
		  std::pair<HypothesisSet::_HCType::iterator, bool> addRet = innerColl.insert(hypo);
		  assert(addRet.second);

		  return StackAdd(true, const_cast<Hypothesis*>(hypoExisting));
		  /*
		  const_cast<Hypothesis*>(hypo)->Swap(*const_cast<Hypothesis*>(hypoExisting));
		  return StackAdd(true, const_cast<Hypothesis*>(hypo));
		  */
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, const_cast<Hypothesis*>(hypo));
	  }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		const HypothesisSet::_HCType &hypos = val.second.GetColl();
		ret.insert(ret.end(), hypos.begin(), hypos.end());
  }

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
	size_t ret = 0;
	BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		const HypothesisSet::_HCType &hypos = val.second.GetColl();
		ret += hypos.size();
	}
	return ret;
}

HypothesisSet &Stack::GetHypothesisSet(const HypoCoverage &key)
{
	/*
	_HCType *ret;
	Coll::iterator iter = m_coll.find(key);
	if (iter == m_coll.end()) {
		ret = new _HCType();
		m_coll[key] = ret;
	}
	else {
		ret = iter->second;
	}
	return *ret;
	*/
	return m_coll[key];
}

}

