#include <boost/foreach.hpp>
#include <sstream>
#include "Hypothesis.h"
#include "Manager.h"
#include "ActiveChart.h"
#include "TargetPhraseImpl.h"
#include "../System.h"
#include "../Scores.h"
#include "../InputPathBase.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
Hypothesis *Hypothesis::Create(MemPool &pool, Manager &mgr)
{
  //  ++g_numHypos;
	SCFG::Hypothesis *ret;
    //ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);

    Recycler<HypothesisBase*> &recycler = mgr.GetHypoRecycle();
    ret = &recycler.Get()->Cast<SCFG::Hypothesis>();
    if (ret) {
      // got new hypo from recycler. Do nothing
    }
    else {
      ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);
      //cerr << "Hypothesis=" << sizeof(Hypothesis) << " " << ret << endl;
      recycler.Keep(ret);
    }
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
  m_path = &path;

  m_scores->Reset(mgr.system);
  m_scores->PlusEquals(mgr.system, GetTargetPhrase().GetScores());

  //cerr << "tp=" << tp << endl;
  //cerr << "symbolBind=" << symbolBind << endl;
  //cerr << endl;
  m_prevHypos.resize(symbolBind.numNT);

  size_t currInd = 0;
  for (size_t i = 0; i < symbolBind.coll.size(); ++i) {
    const SymbolBindElement &ele = symbolBind.coll[i];
    //cerr << "ele=" << ele.word->isNonTerminal << " " << ele.hypos << endl;

    if (ele.hypos) {
      const Hypotheses &sortedHypos = *ele.hypos;

      size_t prevHyposInd = prevHyposIndices[currInd];
      assert(prevHyposInd < sortedHypos.size());

      const Hypothesis &prevHypo = sortedHypos[prevHyposInd]->Cast<SCFG::Hypothesis>();
      m_prevHypos[currInd] = &prevHypo;

      m_scores->PlusEquals(mgr.system, prevHypo.GetScores());

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
  //cerr << "tp=" << tp.Debug(m_mgr->system) << endl;

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
      word.OutputToStream(out);
      out << " ";
    }

  }
}

std::string Hypothesis::Debug(const System &system) const
{
  stringstream out;
  out << this << flush;

  out << " RANGE:";
  out << m_path->range << flush;

  // score
  out << " SCORE:" << GetScores().Debug(GetManager().system) << flush;

  out << m_targetPhrase->Debug(GetManager().system);

  out << "PREV:";
  for (size_t i = 0; i < m_prevHypos.size(); ++i) {
    const Hypothesis *prevHypo = m_prevHypos[i];
    out << prevHypo << " ";
  }
  out << endl;

  /*
  // recursive
  for (size_t i = 0; i < m_prevHypos.size(); ++i) {
    const Hypothesis *prevHypo = m_prevHypos[i];
    out << prevHypo->Debug(GetManager().system) << " ";
  }
  */

  return out.str();
}

} // namespaces
}

