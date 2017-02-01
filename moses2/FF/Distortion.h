/*
 * Distortion.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef DISTORTION_H_
#define DISTORTION_H_

#include "StatefulFeatureFunction.h"
#include "../legacy/Range.h"
#include "../TypeDef.h"

namespace Moses2
{

class Distortion: public StatefulFeatureFunction
{
public:
  Distortion(size_t startInd, const std::string &line);
  virtual ~Distortion();

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

  virtual void EvaluateWhenApplied(const std::deque<Hypothesis*> &hypos) const {
  }

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
                                   const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                   FFState &state) const;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
                                   const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                   FFState &state) const;

protected:
  SCORE CalculateDistortionScore(const Range &prev, const Range &curr,
                                 const int FirstGap) const;

  int ComputeDistortionDistance(const Range& prev, const Range& current) const;

};

}

#endif /* DISTORTION_H_ */
