#include "Stacks.h"
#include "Hypothesis.h"
#include "TargetPhraseImpl.h"
#include "Manager.h"

namespace Moses2
{

namespace SCFG
{

Stack::Stack(const Manager &mgr)
:m_mgr(mgr)
{
}

void Stack::Add(SCFG::Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
    ArcLists &arcLists)
{
  const SCFG::TargetPhraseImpl &tp = hypo->GetTargetPhrase();
  const SCFG::Word &lhs = tp.lhs;
  HypothesisColl &coll = GetMiniStack(lhs);
  StackAdd added = coll.Add(hypo);

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

}
}
