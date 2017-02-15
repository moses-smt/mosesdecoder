/*
 * KENLM.h
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#ifdef __linux
#include <pthread.h>
#endif

#include "../FF/StatefulFeatureFunction.h"
#include "lm/model.hh"
#include "../legacy/Factor.h"
#include "../legacy/Util2.h"
#include "../Word.h"
#include "../TypeDef.h"

namespace Moses2
{

class Word;

class GPULM: public StatefulFeatureFunction
{
public:
  GPULM(size_t startInd, const std::string &line);

  virtual ~GPULM();

  virtual void Load(System &system);

  void SetParameter(const std::string& key,
                    const std::string& value);

  virtual FFState* BlankState(MemPool &pool, const System &sys) const;

  //! return the state associated with the empty hypothesis for a given sentence
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

  virtual void EvaluateWhenAppliedBatch(
    const System &system,
    const Batch &batch) const;

protected:
  std::string m_path;
  FactorType m_factorType;
  util::LoadMethod m_load_method;
  const Factor *m_bos;
  const Factor *m_eos;
  size_t m_order;

  inline lm::WordIndex TranslateID(const Word &word) const {
    std::size_t factor = word[m_factorType]->GetId();
    return (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
  }

  std::vector<lm::WordIndex> m_lmIdLookup;

  // batch
  void CreateNGram(std::vector<std::pair<Hypothesis*, Context> > &contexts, Hypothesis &hypo) const;

  void ShiftOrPush(std::vector<const Factor*> &context,
                   const Factor *factor) const;

  SCORE Score(const Context &context) const;
};

}
