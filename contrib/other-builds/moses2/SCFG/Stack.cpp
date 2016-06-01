#include <boost/foreach.hpp>
#include "Stacks.h"
#include "Hypothesis.h"
#include "TargetPhraseImpl.h"
#include "Manager.h"

using namespace std;

namespace Moses2
{

namespace SCFG
{

Stack::Stack(const Manager &mgr)
:m_mgr(mgr)
{
}

Stack::~Stack()
{
  BOOST_FOREACH (const Coll::value_type &valPair, m_coll) {
    Moses2::HypothesisColl *hypos = valPair.second;
    delete hypos;
  }
}

void Stack::Add(SCFG::Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
    ArcLists &arcLists)
{
  const SCFG::TargetPhraseImpl &tp = hypo->GetTargetPhrase();
  const SCFG::Word &lhs = tp.lhs;
  //cerr << "lhs=" << lhs << endl;

  HypothesisColl &coll = GetMiniStack(lhs);
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

Moses2::HypothesisColl &Stack::GetMiniStack(const SCFG::Word &key)
{
  Moses2::HypothesisColl *ret;
  Coll::iterator iter;
  iter = m_coll.find(key);
  if (iter == m_coll.end()) {
    ret = new Moses2::HypothesisColl(m_mgr);
    m_coll[key] = ret;
  }
  else {
    ret = iter->second;
  }
  return *ret;
}

size_t Stack::GetSize() const
{
  size_t ret = 0;
  BOOST_FOREACH (const Coll::value_type &valPair, m_coll) {
    Moses2::HypothesisColl &hypos = *valPair.second;
    ret += hypos.GetSize();
  }
  return ret;
}

const Moses2::HypothesisColl *Stack::GetColl(SCFG::Word &nt) const
{
  assert(nt.isNonTerminal);
  Coll::const_iterator iter = m_coll.find(nt);
  if (iter != m_coll.end()) {
    return NULL;
  }
  else {
    return iter->second;
  }
}

const Hypothesis *Stack::GetBestHypo(
    const Manager &mgr,
    ArcLists &arcLists) const
{
  const Hypothesis *ret = NULL;
  BOOST_FOREACH (const Coll::value_type &valPair, m_coll) {
    Moses2::HypothesisColl &hypos = *valPair.second;
    const Hypotheses &sortedHypos = hypos.GetSortedAndPruneHypos(mgr, arcLists);
    const Hypothesis *bestHypoColl = static_cast<const Hypothesis*>(sortedHypos[0]);

    if (ret == NULL || ret->GetFutureScore() < bestHypoColl->GetFutureScore()) {
      ret = bestHypoColl;
    }
  }

  return ret;
}

}
}
