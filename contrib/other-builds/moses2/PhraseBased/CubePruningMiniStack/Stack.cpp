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
  BOOST_FOREACH(const Coll::value_type &val, m_coll){
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
  StackAdd added = coll.Add(hypo);

  size_t nbestSize = m_mgr.system.options.nbest.nbest_size;
  if (nbestSize) {
    arcLists.AddArc(added.added, hypo, added.other);
  }
  else {
    if (!added.added) {
      hypoRecycle.Recycle(hypo);
    }
    else if (added.other) {
      hypoRecycle.Recycle(added.other);
    }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret;
  BOOST_FOREACH(const Coll::value_type &val, m_coll){
  const Moses2::HypothesisColl::_HCType &hypos = val.second->GetColl();

  ret.reserve(ret.size() + hypos.size());
  BOOST_FOREACH(const HypothesisBase *hypo, hypos) {
    ret.push_back(static_cast<const Hypothesis*>(hypo));
  }
}

  std::vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num) ? ret.end() : ret.begin() + num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
      HypothesisFutureScoreOrderer());

  return ret;
}

size_t Stack::GetHypoSize() const
{
  size_t ret = 0;
  BOOST_FOREACH(const Coll::value_type &val, m_coll){
  const Moses2::HypothesisColl::_HCType &hypos = val.second->GetColl();
  ret += hypos.size();
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
  BOOST_FOREACH(const Coll::value_type &val, m_coll){
  Moses2::HypothesisColl *miniStack = val.second;
  m_miniStackRecycler.push_back(miniStack);
}

m_coll.clear();
}

void Stack::DebugCounts()
{
  cerr << "counts=";
  BOOST_FOREACH(const Coll::value_type &val, GetColl()){
  const Moses2::HypothesisColl &miniStack = *val.second;
  size_t count = miniStack.GetColl().size();
  cerr << count << " ";
}
  cerr << endl;
}

}

}

