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

StackAdd Stack::Add(SCFG::Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
    ArcLists &arcLists)
{
  const SCFG::TargetPhraseImpl &tp = hypo->GetTargetPhrase();
  const SCFG::Word &lhs = tp.lhs;
  cerr << "lhs=" << lhs << endl;

  HypothesisColl &coll = GetMiniStack(lhs);
  StackAdd added = coll.Add(hypo);
  return added;
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

}
}
