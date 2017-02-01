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

  HypothesisColl &coll = GetColl(lhs);
  coll.Add(m_mgr, hypo, hypoRecycle, arcLists);
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

const Moses2::HypothesisColl *Stack::GetColl(const SCFG::Word &nt) const
{
  assert(nt.isNonTerminal);
  Coll::const_iterator iter = m_coll.find(nt);
  if (iter != m_coll.end()) {
    return NULL;
  } else {
    return iter->second;
  }
}

Moses2::HypothesisColl &Stack::GetColl(const SCFG::Word &nt)
{
  Moses2::HypothesisColl *ret;
  Coll::iterator iter;
  iter = m_coll.find(nt);
  if (iter == m_coll.end()) {
    ret = new Moses2::HypothesisColl(m_mgr);
    m_coll[nt] = ret;
  } else {
    ret = iter->second;
  }
  return *ret;
}

const Hypothesis *Stack::GetBestHypo() const
{
  SCORE bestScore = -std::numeric_limits<SCORE>::infinity();
  const HypothesisBase *bestHypo = NULL;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
    const Moses2::HypothesisColl &hypos = *val.second;
    const Moses2::HypothesisBase *hypo = hypos.GetBestHypo();

    if (hypo->GetFutureScore() > bestScore) {
      bestScore = hypo->GetFutureScore();
      bestHypo = hypo;
    }
  }
  return &bestHypo->Cast<SCFG::Hypothesis>();
}

std::string Stack::Debug(const System &system) const
{
  stringstream out;
  BOOST_FOREACH (const SCFG::Stack::Coll::value_type &valPair, m_coll) {
    const SCFG::Word &lhs = valPair.first;
    const Moses2::HypothesisColl &hypos = *valPair.second;
    out << "lhs=" << lhs.Debug(system);
    out << "=" << hypos.GetSize() << endl;
    out << hypos.Debug(system);
    out << endl;
  }

  return out.str();
}

}
}
