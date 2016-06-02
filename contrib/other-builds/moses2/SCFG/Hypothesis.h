#pragma once
#include <vector>
#include "../HypothesisBase.h"
#include "../MemPool.h"

namespace Moses2
{
class InputPathBase;
class StatefulFeatureFunction;

namespace SCFG
{
class TargetPhraseImpl;
class Manager;
class SymbolBind;
class InputPath;

class Hypothesis: public HypothesisBase
{
  friend std::ostream& operator<<(std::ostream &, const SCFG::Hypothesis &);

public:
  static Hypothesis *Create(MemPool &pool, Manager &mgr);

  Hypothesis(MemPool &pool,
      const System &system);

  void Init(SCFG::Manager &mgr,
      const SCFG::InputPath &path,
      const SCFG::SymbolBind &symbolBind,
      const SCFG::TargetPhraseImpl &tp,
      const std::vector<size_t> &prevHyposIndices);

  virtual SCORE GetFutureScore() const;
  virtual void EvaluateWhenApplied();

  const SCFG::TargetPhraseImpl &GetTargetPhrase() const
  {  return *m_targetPhrase; }

  const SCFG::SymbolBind &GetSymbolBind() const
  {  return *m_symbolBind; }

  //! get a particular previous hypos
  const Hypothesis* GetPrevHypo(size_t ind) const {
    return m_prevHypos[ind];
  }

  void OutputToStream(std::ostream &out) const;
  virtual std::string Debug() const;

protected:
  const SCFG::TargetPhraseImpl *m_targetPhrase;
  const InputPathBase *m_path;
  const SCFG::SymbolBind *m_symbolBind;

  Vector<const Hypothesis*> m_prevHypos; // always sorted by source position?

  void EvaluateWhenApplied(const StatefulFeatureFunction &sfff);

};

}
}

