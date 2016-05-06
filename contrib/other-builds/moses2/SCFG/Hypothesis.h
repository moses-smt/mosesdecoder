#pragma once
#include <vector>
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
  friend std::ostream& operator<<(std::ostream &, const Hypothesis &);

public:
  Hypothesis(MemPool &pool,
      const System &system);

  void Init(SCFG::Manager &mgr,
      const InputPathBase &path,
      const SCFG::SymbolBind &symbolBind,
      const SCFG::TargetPhraseImpl &tp,
      const std::vector<size_t> &prevHyposIndices);

  virtual SCORE GetFutureScore() const;
  virtual void EvaluateWhenApplied();

  const SCFG::TargetPhraseImpl &GetTargetPhrase() const
  {  return *m_targetPhrase; }

  //! vector of previous hypotheses this hypo is built on
  const std::vector<const Hypothesis*> &GetPrevHypos() const {
    return m_prevHypos;
  }

  //! get a particular previous hypos
  const Hypothesis* GetPrevHypo(size_t ind) const {
    return m_prevHypos[ind];
  }

  void OutputToStream(std::ostream &out) const;

protected:
  const SCFG::TargetPhraseImpl *m_targetPhrase;
  const InputPathBase *m_path;
  const SCFG::SymbolBind *m_symbolBind;

  std::vector<const Hypothesis*> m_prevHypos; // always sorted by source position?

};

}
}

