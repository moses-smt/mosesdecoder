/*
 * LexicalReordering.h
 *
 *  Created on: 15 Dec 2015
 *      Author: hieu
 */

#pragma once
#include "StatefulFeatureFunction.h"

namespace Moses2 {

class LexicalReordering : public StatefulFeatureFunction
{
public:
  LexicalReordering(size_t startInd, const std::string &line);
  virtual ~LexicalReordering();

  virtual void Load(System &system);

  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual FFState* BlankState(const Manager &mgr, const InputType &input) const;
  virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const InputType &input) const;

  virtual void
  EvaluateInIsolation(const System &system,
		  const Phrase &source,
		  const TargetPhrase &targetPhrase,
		  Scores &scores,
		  Scores *estimatedScores) const
  {}

  virtual void EvaluateWhenApplied(const Manager &mgr,
	const Hypothesis &hypo,
	const FFState &prevState,
	Scores &scores,
	FFState &state) const;

protected:
  std::string m_path;
};

} /* namespace Moses2 */

