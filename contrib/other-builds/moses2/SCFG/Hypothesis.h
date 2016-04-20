#pragma once

#include "../HypothesisBase.h"

namespace Moses2
{
class InputPathBase;

namespace SCFG
{
class TargetPhraseImpl;
class Manager;
class SymbolBind;

class Hypothesis: public HypothesisBase
{

public:
  Hypothesis(MemPool &pool, const System &system);

  void Init(SCFG::Manager &mgr,
      const InputPathBase &path,
      const SCFG::SymbolBind &symbolBind,
      const SCFG::TargetPhraseImpl &tp);

  virtual SCORE GetFutureScore() const;
  virtual void EvaluateWhenApplied();

  const SCFG::TargetPhraseImpl &GetTargetPhrase() const
  {
    return *m_targetPhrase;
  }

protected:
  const SCFG::TargetPhraseImpl *m_targetPhrase;
  const InputPathBase *m_path;
  const SCFG::SymbolBind *m_symbolBind;
};

}
}

