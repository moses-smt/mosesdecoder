#include <boost/foreach.hpp>
#include <sstream>
#include "Hypothesis.h"
#include "Manager.h"
#include "ActiveChart.h"
#include "TargetPhraseImpl.h"
#include "../System.h"
#include "../Scores.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
Hypothesis *Hypothesis::Create(MemPool &pool, Manager &mgr)
{
  //  ++g_numHypos;
    Hypothesis *ret;
    ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);

    /*
    Recycler<HypothesisBase*> &recycler = mgr.GetHypoRecycle();
    ret = static_cast<Hypothesis*>(recycler.Get());
    if (ret) {
      // got new hypo from recycler. Do nothing
    }
    else {
      ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);
      //cerr << "Hypothesis=" << sizeof(Hypothesis) << " " << ret << endl;
      recycler.Keep(ret);
    }
    */
    return ret;
}

Hypothesis::Hypothesis(MemPool &pool,
    const System &system)
:HypothesisBase(pool, system)
,m_prevHypos(pool)
{

}

void Hypothesis::Init(SCFG::Manager &mgr,
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind,
    const SCFG::TargetPhraseImpl &tp,
    const Vector<size_t> &prevHyposIndices)
{
  m_mgr = &mgr;
  m_targetPhrase = &tp;

  //cerr << "tp=" << tp << endl;
  //cerr << "symbolBind=" << symbolBind << endl;
  //cerr << endl;
  m_prevHypos.resize(symbolBind.numNT);

  size_t currInd = 0;
  for (size_t i = 0; i < symbolBind.coll.size(); ++i) {
    const SymbolBindElement &ele = symbolBind.coll[i];
    //cerr << "ele=" << ele.word->isNonTerminal << " " << ele.hypos << endl;

    if (ele.hypos) {
      const Hypotheses &sortedHypos = ele.hypos->GetSortedAndPruneHypos(mgr, mgr.arcLists);

      size_t prevHyposInd = prevHyposIndices[currInd];
      assert(prevHyposInd < sortedHypos.size());

      const Hypothesis *prevHypo = static_cast<const SCFG::Hypothesis*>(sortedHypos[prevHyposInd]);
      m_prevHypos[currInd] = prevHypo;

      ++currInd;
    }
  }
}

SCORE Hypothesis::GetFutureScore() const
{
  return GetScores().GetTotalScore();
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*> &sfffs =
      GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs){
    EvaluateWhenApplied(*sfff);
  }
//cerr << *this << endl;

}

void Hypothesis::EvaluateWhenApplied(const StatefulFeatureFunction &sfff)
{
  const SCFG::Manager &mgr = static_cast<const SCFG::Manager&>(GetManager());
  size_t statefulInd = sfff.GetStatefulInd();
  FFState *thisState = m_ffStates[statefulInd];
  sfff.EvaluateWhenApplied(mgr, *this, statefulInd, GetScores(),
      *thisState);

}

void Hypothesis::OutputToStream(std::ostream &out) const
{
  const SCFG::TargetPhraseImpl &tp = GetTargetPhrase();
  //cerr << "tp=" << tp.GetSize() << tp << endl;

  for (size_t pos = 0; pos < tp.GetSize(); ++pos) {
    const SCFG::Word &word = tp[pos];
    //cerr << "word " << pos << "=" << word << endl;
    if (word.isNonTerminal) {
      //cerr << "is nt" << endl;
      // non-term. fill out with prev hypo
      size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[pos];
      const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
      prevHypo->OutputToStream(out);
    }
    else {
      //cerr << "not nt" << endl;
      out << word << " ";
    }

  }
}

std::ostream& operator<<(std::ostream &out, const SCFG::Hypothesis &obj)
{
  out << &obj;

  out << " m_prevHypos=" << obj.m_prevHypos.size() << " ";
  for (size_t i = 0; i < obj.m_prevHypos.size(); ++i) {
    out << obj.m_prevHypos[i] << " ";
  }
  //obj.OutputToStream(out);

  return out;
}

std::string Hypothesis::Debug() const
{
  std::stringstream strm;
  strm << *this;
  return strm.str();
}

} // namespaces
}

