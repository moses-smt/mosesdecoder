/*
 * LanguageModel.h
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "../FF/StatefulFeatureFunction.h"
#include "../TypeDef.h"
#include "../InMemoryTrie/InMemoryTrie.h"
#include "../legacy/Factor.h"
#include "../legacy/Util2.h"

namespace Moses2
{

////////////////////////////////////////////////////////////////////////////////////////
struct LMScores {
  LMScores() {
  }

  LMScores(const LMScores &copy) :
    prob(copy.prob), backoff(copy.backoff) {
  }

  LMScores(float inProb, float inBackoff) :
    prob(inProb), backoff(inBackoff) {
  }

  void Debug(std::ostream &out, const System &system) const {
    out << "(" << prob << "," << backoff << ")" << std::flush;
  }

  float prob, backoff;
};

////////////////////////////////////////////////////////////////////////////////////////
class LanguageModel: public StatefulFeatureFunction
{
public:
  LanguageModel(size_t startInd, const std::string &line);
  virtual ~LanguageModel();

  virtual void Load(System &system);

  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual FFState* BlankState(MemPool &pool, const System &sys) const;
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                    const InputType &input, const Hypothesis &hypo) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
                      const TargetPhraseImpl &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
                                   const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                   FFState &state) const;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
                                   const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                   FFState &state) const;

protected:
  std::string m_path;
  FactorType m_factorType;
  size_t m_order;

  InMemoryTrie<const Factor*, LMScores> m_root;
  SCORE m_oov;
  const Factor *m_bos;
  const Factor *m_eos;

  void ShiftOrPush(std::vector<const Factor*> &context,
                   const Factor *factor) const;
  std::pair<SCORE, void*> Score(
    const std::vector<const Factor*> &context) const;
  SCORE BackoffScore(const std::vector<const Factor*> &context) const;

  void DebugContext(const std::vector<const Factor*> &context) const;
};

}

