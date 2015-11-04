/*
 * KENLM.h
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */

#ifndef FF_LM_KENLM_H_
#define FF_LM_KENLM_H_

#include <boost/shared_ptr.hpp>
#include "../FF/StatefulFeatureFunction.h"
#include "lm/model.hh"
#include "moses/Factor.h"

class Word;

class KENLM : public StatefulFeatureFunction
{
public:
  KENLM(size_t startInd, const std::string &line);
  virtual ~KENLM();

  virtual void Load(System &system);

  //! return the state associated with the empty hypothesis for a given sentence
  virtual const Moses::FFState* EmptyHypothesisState(const Manager &mgr, const PhraseImpl &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScores) const;

  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
	const Hypothesis &hypo,
	const Moses::FFState &prevState,
	Scores &scores) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::string m_path;
  Moses::FactorType m_factorType;
  const Moses::Factor *m_bos;
  const Moses::Factor *m_eos;

  typedef lm::ngram::ProbingModel Model;
  boost::shared_ptr<Model> m_ngram;
  std::vector<lm::WordIndex> m_lmIdLookup;

  void CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const;

  lm::WordIndex TranslateID(const Word &word) const;
  lm::WordIndex *LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const;

};

#endif /* FF_LM_KENLM_H_ */
