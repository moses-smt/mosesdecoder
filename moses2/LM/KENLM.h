/*
 * KENLM.h
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#pragma once
#include <boost/shared_ptr.hpp>
#include "../FF/StatefulFeatureFunction.h"
#include "lm/model.hh"
#include "../legacy/Factor.h"
#include "../legacy/Util2.h"
#include "../Word.h"

namespace Moses2
{

class Word;

FeatureFunction *ConstructKenLM(size_t startInd, const std::string &lineOrig);
FeatureFunction *ConstructKenLM(size_t startInd, const std::string &line,
                                const std::string &file, FactorType factorType,
                                util::LoadMethod load_method);

template<class Model>
class KENLM: public StatefulFeatureFunction
{
public:
  KENLM(size_t startInd, const std::string &line, const std::string &file,
        FactorType factorType, util::LoadMethod load_method);

  virtual ~KENLM();

  virtual void Load(System &system);

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

protected:
  std::string m_path;
  FactorType m_factorType;
  util::LoadMethod m_load_method;
  const Factor *m_bos;
  const Factor *m_eos;

  boost::shared_ptr<Model> m_ngram;

  void CalcScore(const Phrase<Moses2::Word> &phrase, float &fullScore, float &ngramScore,
                 std::size_t &oovCount) const;

  void CalcScore(const Phrase<SCFG::Word> &phrase, float &fullScore, float &ngramScore,
                 std::size_t &oovCount) const;

  inline lm::WordIndex TranslateID(const Word &word) const {
    std::size_t factor = word[m_factorType]->GetId();
    return (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
  }
  // Convert last words of hypothesis into vocab ids, returning an end pointer.
  lm::WordIndex *LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const;

  std::vector<lm::WordIndex> m_lmIdLookup;

};

}

