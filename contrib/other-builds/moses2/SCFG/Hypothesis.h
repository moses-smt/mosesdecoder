#pragma once

#include "../HypothesisBase.h"

namespace Moses2
{
class InputPathBase;

namespace SCFG
{
class TargetPhrase;
class Manager;

class Hypothesis: public HypothesisBase
{

public:
  Hypothesis(MemPool &pool, const System &system);

  void Init(SCFG::Manager &mgr, const InputPathBase &path, const SCFG::TargetPhrase &tp);

  virtual SCORE GetFutureScore() const;
  virtual void EvaluateWhenApplied();

  const TargetPhrase &GetTargetPhrase() const
  {
    return *m_targetPhrase;
  }

protected:
  const SCFG::TargetPhrase *m_targetPhrase;
  const InputPathBase *m_path;

};

}
}

