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
  ,m_coll()
{
}

Stack::~Stack()
{
  // TODO Auto-generated destructor stub
}

void Stack::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
  std::pair<_HCType::iterator, bool> addRet = m_coll.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
    // equiv hypo doesn't exists
  } else {
    const Hypothesis *hypoExisting = *addRet.first;
    if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
      // incoming hypo is better than the one we have
      const Hypothesis *const &hypoExisting1 = *addRet.first;
      const Hypothesis *&hypoExisting2 = const_cast<const Hypothesis *&>(hypoExisting1);
      hypoExisting2 = hypo;

      Hypothesis *hypoToBeDeleted = const_cast<Hypothesis*>(hypoExisting);
      hypoRecycle.Recycle(hypoToBeDeleted);
    } else {
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

  MemPool &pool = mgr.GetPool();

  // prune and sort
  Hypotheses *allHypos = new (pool.Allocate<Hypotheses>()) Hypotheses(pool, GetHypoSize());
  size_t i = 0;
  BOOST_FOREACH(const Hypothesis *hypo, m_coll) {
    (*allHypos)[i++] = hypo;
  }
  SortAndPruneHypos(mgr, *allHypos);

  // divide hypos by [bitmap, last end pos]
  BOOST_FOREACH(const Hypothesis *hypo, *allHypos) {
    HypoCoverage key(&hypo->GetBitmap(), hypo->GetInputPath().range.GetEndPos());

    Hypotheses *hypos;
    SortedHypos::iterator iter;
    iter = ret.find(key);
    if (iter == ret.end()) {
      hypos = new (pool.Allocate<Hypotheses>()) Hypotheses(pool);
      ret[key] = hypos;
    } else {
      hypos = iter->second;
    }
    hypos->push_back(hypo);
  }

  return ret;
}


//Stack::SortedHypos Stack::GetSortedAndPruneHypos(const Manager &mgr) const
//{
//  SortedHypos ret;
//
//  MemPool &pool = mgr.GetPool();
//
//  // divide hypos by [bitmap, last end pos]
//  BOOST_FOREACH(const Hypothesis *hypo, m_coll) {
//	  HypoCoverage key(&hypo->GetBitmap(), hypo->GetInputPath().range.GetEndPos());
//
//	  Hypotheses *hypos;
//	  SortedHypos::iterator iter;
//	  iter = ret.find(key);
//	  if (iter == ret.end()) {
//		  hypos = new (pool.Allocate<Hypotheses>()) Hypotheses(pool);
//		  ret[key] = hypos;
//	  }
//	  else {
//		  hypos = iter->second;
//	  }
//	  hypos->push_back(hypo);
//  }
//
//  // put into real return variable and sort
//  BOOST_FOREACH(SortedHypos::value_type &val, ret) {
//	  Hypotheses &hypos = *val.second;
//	  SortAndPruneHypos(mgr, hypos);
//  }
//
//  return ret;
//}

void Stack::SortAndPruneHypos(const Manager &mgr, Hypotheses &hypos) const
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
  iterMiddle = (stackSize == 0 || hypos.size() < stackSize)
               ? hypos.end()
               : hypos.begin() + stackSize;

  std::partial_sort(hypos.begin(), iterMiddle, hypos.end(),
                    HypothesisFutureScoreOrderer());

  // prune
  if (stackSize && hypos.size() > stackSize) {
    for (size_t i = stackSize; i < hypos.size(); ++i) {
      Hypothesis *hypo = const_cast<Hypothesis*>(hypos[i]);
      recycler.Recycle(hypo);
    }
    hypos.resize(stackSize);
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

