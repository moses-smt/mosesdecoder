#include "Hypothesis.h"
#include "Manager.h"

namespace Moses2
{
namespace SCFG
{
Hypothesis::Hypothesis(MemPool &pool, const System &system)
:HypothesisBase(pool, system)
{

}

void Hypothesis::Init(SCFG::Manager &mgr, const InputPathBase &path, const SCFG::TargetPhraseImpl &tp)
{
  m_mgr = &mgr;
  m_targetPhrase = &tp;

}

SCORE Hypothesis::GetFutureScore() const
{

}

void Hypothesis::EvaluateWhenApplied()
{

}

}
}

