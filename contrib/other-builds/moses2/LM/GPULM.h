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
#include <pthread.h>

#include "../FF/StatefulFeatureFunction.h"
#include "lm/model.hh"
#include "../legacy/Factor.h"
#include "../legacy/Util2.h"
#include "../Word.h"
#include "../TypeDef.h"

#include "LM/gpuLM.hh"
#include <utility>

namespace Moses2
{

class Word;

class GPULM: public StatefulFeatureFunction
{
  mutable boost::thread_specific_ptr<float> m_results;
  mutable boost::thread_specific_ptr<unsigned int> m_ngrams_for_query;
  
  size_t max_num_queries;
  unsigned short max_ngram_order;
  std::unordered_map<const Factor *, unsigned int> encode_map;
public:
  GPULM(size_t startInd, const std::string &line);
  float * getThreadLocalResults() const;
  unsigned int * getThreadLocalngrams() const;

  virtual ~GPULM();

  virtual void Load(System &system);

  void SetParameter(const std::string& key,
      const std::string& value);

  virtual FFState* BlankState(MemPool &pool) const;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
      const InputType &input, const Hypothesis &hypo) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
      const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
      SCORE *estimatedScore) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
      const Hypothesis &hypo, const FFState &prevState, Scores &scores,
      FFState &state) const;

  virtual void EvaluateWhenAppliedBatch(
      const System &system,
      const Batch &batch) const;

protected:
  gpuLM *m_obj;

  std::string m_path;
  FactorType m_factorType;
  util::LoadMethod m_load_method;
  const Factor *m_bos;
  const Factor *m_eos;
  size_t m_order;

  // batch
  void CreateNGram(std::vector<std::pair<Hypothesis*, Context> > &contexts, Hypothesis &hypo) const;

  void ShiftOrPush(std::vector<const Factor*> &context,
      const Factor *factor) const;

  void CreateQueryVec(
		  const Context &context,
		  unsigned int &position) const;

};

}
