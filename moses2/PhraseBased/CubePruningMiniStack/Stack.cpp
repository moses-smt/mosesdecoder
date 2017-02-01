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

namespace NSCubePruningMiniStack
{
Stack::Stack(const Manager &mgr) :
  m_mgr(mgr), m_coll(
    MemPoolAllocator<std::pair<HypoCoverage, Moses2::HypothesisColl*> >(
      mgr.GetPool())), m_miniStackRecycler(
        MemPoolAllocator<Moses2::HypothesisColl*>(mgr.GetPool()))
{
}

Stack::~Stack()
{
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
    const Moses2::HypothesisColl *miniStack = val.second;
    delete miniStack;
  }

  while (!m_miniStackRecycler.empty()) {
    Moses2::HypothesisColl *miniStack = m_miniStackRecycler.back();
    m_miniStackRecycler.pop_back();
    delete miniStack;

  }
}

void Stack::Add(Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
                ArcLists &arcLists)
{
  HypoCoverage key(&hypo->GetBitmap(), hypo->GetInputPath().range.GetEndPos());
  Moses2::HypothesisColl &coll = GetMiniStack(key);
  coll.Add(m_mgr, hypo, hypoRecycle, arcLists);
}

const Hypothesis *Stack::GetBestHypo() const
{
  SCORE bestScore = -std::numeric_limits<SCORE>::infinity();
  const HypothesisBase *bestHypo = NULL;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
    const Moses2::HypothesisColl &hypos = *val.second;
    const Moses2::HypothesisBase *hypo = hypos.GetBestHypo();

    if (hypo && hypo->GetFutureScore() > bestScore) {
      bestScore = hypo->GetFutureScore();
      bestHypo = hypo;
    }
  }
  return &bestHypo->Cast<Hypothesis>();
}

size_t Stack::GetHypoSize() const
{
  size_t ret = 0;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
    const Moses2::HypothesisColl &hypos = *val.second;
    ret += hypos.GetSize();
  }
  return ret;
}

Moses2::HypothesisColl &Stack::GetMiniStack(const HypoCoverage &key)
{
  Moses2::HypothesisColl *ret;
  Coll::iterator iter = m_coll.find(key);
  if (iter == m_coll.end()) {
    if (m_miniStackRecycler.empty()) {
      ret = new Moses2::HypothesisColl(m_mgr);
    } else {
      ret = m_miniStackRecycler.back();
      ret->Clear();
      m_miniStackRecycler.pop_back();
    }

    m_coll[key] = ret;
  } else {
    ret = iter->second;
  }
  return *ret;
}

void Stack::Clear()
{
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
    Moses2::HypothesisColl *miniStack = val.second;
    m_miniStackRecycler.push_back(miniStack);
  }

  m_coll.clear();
}

void Stack::DebugCounts()
{
  cerr << "counts=";
  BOOST_FOREACH(const Coll::value_type &val, GetColl()) {
    const Moses2::HypothesisColl &miniStack = *val.second;
    size_t count = miniStack.GetSize();
    cerr << count << " ";
  }
  cerr << endl;
}

}

}

