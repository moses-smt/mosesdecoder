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

namespace NSCubePruning
{
MiniStack::MiniStack(const Manager &mgr)
:m_coll(MemPoolAllocator<const Hypothesis*>(mgr.GetPool()))
,m_sortedHypos(NULL)
{}

StackAdd MiniStack::Add(const Hypothesis *hypo)
{
  std::pair<_HCType::iterator, bool> addRet = m_coll.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
	// equiv hypo doesn't exists
	return StackAdd(true, NULL);
  }
  else {
	  const Hypothesis *hypoExisting = *addRet.first;
	  if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
		  // incoming hypo is better than the one we have
		  const Hypothesis *const &hypoExisting1 = *addRet.first;
		  const Hypothesis *&hypoExisting2 = const_cast<const Hypothesis *&>(hypoExisting1);
		  hypoExisting2 = hypo;

		  //hypoExisting->Prefetch();

		  return StackAdd(true, const_cast<Hypothesis*>(hypoExisting));
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, const_cast<Hypothesis*>(hypo));
	  }
  }

  assert(false);
}

Hypotheses &MiniStack::GetSortedAndPruneHypos(const Manager &mgr) const
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

void MiniStack::SortAndPruneHypos(const Manager &mgr) const
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

void MiniStack::Clear()
{
	m_sortedHypos = NULL;
	m_coll.clear();
}

///////////////////////////////////////////////////////////////
Stack::Stack(const Manager &mgr)
:m_mgr(mgr)
,m_coll(MemPoolAllocator< std::pair<HypoCoverage, MiniStack*> >(mgr.GetPool()))
{
}

Stack::~Stack() {
	// TODO Auto-generated destructor stub
}

void Stack::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
  HypoCoverage key(&hypo->GetBitmap(), hypo->GetInputPath().range.GetEndPos());
  StackAdd added = GetMiniStack(key).Add(hypo);

  if (added.toBeDeleted) {
	hypoRecycle.Recycle(added.toBeDeleted);
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		const MiniStack::_HCType &hypos = val.second->GetColl();
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
		const MiniStack::_HCType &hypos = val.second->GetColl();
		ret += hypos.size();
	}
	return ret;
}

MiniStack &Stack::GetMiniStack(const HypoCoverage &key)
{
	MiniStack *ret;
	Coll::iterator iter = m_coll.find(key);
	if (iter == m_coll.end()) {
		if (m_miniStackRecycler.empty()) {
			ret = new (m_mgr.GetPool().Allocate<MiniStack>()) MiniStack(m_mgr);
		}
		else {
			ret = m_miniStackRecycler.back();
			ret->Clear();
			m_miniStackRecycler.pop_back();
		}

		m_coll[key] = ret;
	}
	else {
		ret = iter->second;
	}
	return *ret;
}

void Stack::Clear()
{
	BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		MiniStack *miniStack = val.second;
		m_miniStackRecycler.push_back(miniStack);
	}

	m_coll.clear();
}

void Stack::DebugCounts()
{
	cerr << "counts=";
	BOOST_FOREACH(const Coll::value_type &val, GetColl()) {
		const NSCubePruning::MiniStack &miniStack = *val.second;
		size_t count = miniStack.GetColl().size();
		cerr << count << " ";
	}
	cerr << endl;
}

}

}

