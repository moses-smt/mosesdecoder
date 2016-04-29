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

  //! vector of previous hypotheses this hypo is built on
  const std::vector<const Hypothesis*> &GetPrevHypos() const {
    return m_prevHypos;
  }

  //! get a particular previous hypos
  const Hypothesis* GetPrevHypo(size_t pos) const {
    return m_prevHypos[pos];
  }

protected:
  const SCFG::TargetPhraseImpl *m_targetPhrase;
  const InputPathBase *m_path;
  const SCFG::SymbolBind *m_symbolBind;

  std::vector<const Hypothesis*> m_prevHypos; // always sorted by source position?

};

}
}

