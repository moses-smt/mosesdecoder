/*
 * KENLMBatch.h
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

class Word;

class KENLMBatch : public StatefulFeatureFunction
{
public:
  KENLMBatch(size_t startInd, const std::string &line);
  virtual ~KENLMBatch();

  virtual void Load(System &system);

  virtual FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScores) const;

  virtual void EvaluateBeforeApplied(const Manager &mgr,
    const Hypothesis &hypo,
    const FFState &prevState,
    Scores &scores,
	FFState &state) const;

  virtual void EvaluateWhenApplied(const Manager &mgr,
	const Hypothesis &hypo,
	const FFState &prevState,
	Scores &scores,
	FFState &state) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::string m_path;
  FactorType m_factorType;
  bool m_lazy;
  const Factor *m_bos;
  const Factor *m_eos;

  typedef lm::ngram::ProbingModel Model;
  boost::shared_ptr<Model> m_ngram;
  std::vector<lm::WordIndex> m_lmIdLookup;

  void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const;

  lm::WordIndex TranslateID(const Word &word) const;
  lm::WordIndex *LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const;

};

