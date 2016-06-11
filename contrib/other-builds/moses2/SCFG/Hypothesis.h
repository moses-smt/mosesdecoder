#pragma once
#include <vector>
#include "../HypothesisBase.h"
#include "../MemPool.h"
#include "../Vector.h"

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
public:
  static Hypothesis *Create(MemPool &pool, Manager &mgr);

  void Init(SCFG::Manager &mgr,
      const SCFG::InputPath &path,
      const SCFG::SymbolBind &symbolBind,
      const SCFG::TargetPhraseImpl &tp,
      const Vector<size_t> &prevHyposIndices);

  virtual SCORE GetFutureScore() const;
  virtual void EvaluateWhenApplied();

  const SCFG::TargetPhraseImpl &GetTargetPhrase() const
  {  return *m_targetPhrase; }

  //! get a particular previous hypos
  const Hypothesis* GetPrevHypo(size_t ind) const {
    return m_prevHypos[ind];
  }

  void OutputToStream(std::ostream &out) const;

  void Debug(std::ostream &out, const System &system) const;

protected:
  const SCFG::TargetPhraseImpl *m_targetPhrase;
  const InputPathBase *m_path;

  Vector<const Hypothesis*> m_prevHypos; // always sorted by source position?

  Hypothesis(MemPool &pool,
      const System &system);

  void EvaluateWhenApplied(const StatefulFeatureFunction &sfff);

};

}
}

