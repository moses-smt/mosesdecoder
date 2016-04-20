#include <boost/foreach.hpp>
#include "Hypothesis.h"
#include "Manager.h"
#include "../System.h"
#include "../Scores.h"
#include "../FF/StatefulFeatureFunction.h"

namespace Moses2
{
namespace SCFG
{
Hypothesis::Hypothesis(MemPool &pool, const System &system)
:HypothesisBase(pool, system)
{

}

void Hypothesis::Init(SCFG::Manager &mgr,
    const InputPathBase &path,
    const SCFG::SymbolBind &symbolBind,
    const SCFG::TargetPhraseImpl &tp)
{
  m_mgr = &mgr;
  m_symbolBind = &symbolBind;
  m_targetPhrase = &tp;
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
    //EvaluateWhenApplied(*sfff);
  }
//cerr << *this << endl;

}

}
}

