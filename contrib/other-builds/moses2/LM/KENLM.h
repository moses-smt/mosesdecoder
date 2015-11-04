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
		  Scores *estimatedScore) const;

  virtual Moses::FFState* EvaluateWhenApplied(const Manager &mgr,
	const Hypothesis &hypo,
	const Moses::FFState &prevState,
	Scores &scores) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::string m_path;
  Moses::FactorType m_factorType;

  typedef lm::ngram::ProbingModel Model;
  boost::shared_ptr<Model> m_ngram;
  std::vector<lm::WordIndex> m_lmIdLookup;
};

#endif /* FF_LM_KENLM_H_ */
