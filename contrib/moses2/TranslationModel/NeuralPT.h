#pragma once
#include "StatefulPhraseTable.h"
#include "../legacy/Range.h"

namespace Moses2
{
class NeuralPTState;

struct NeuralPhrase
{
  std::vector<const Factor*> words;
  SCORE score;
  Range range;
};

class NeuralPT : public StatefulPhraseTable
{
public:
  NeuralPT(size_t startInd, const std::string &line);

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
      const TargetPhraseImpl &targetPhrase, Scores &scores,
      SCORE &estimatedScore) const {}

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
      SCORE &estimatedScore) const {}

  //! return uninitialise state
  virtual FFState* BlankState(MemPool &pool, const System &sys) const;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
      const InputType &input, const Hypothesis &hypo) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
      const Hypothesis &hypo, const FFState &prevState, Scores &scores,
      FFState &state) const;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
      const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
      FFState &state) const;

  virtual void BeforeExtending(const Hypotheses &hypos, const Manager &mgr) const;

protected:
  void BeforeExtending(Hypothesis &hypo, const Manager &mgr) const;
  std::vector<NeuralPhrase> Lookup(const NeuralPTState &state) const;

};

}
