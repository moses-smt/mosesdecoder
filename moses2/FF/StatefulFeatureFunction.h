/*
 * StatefulFeatureFunction.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef STATEFULFEATUREFUNCTION_H_
#define STATEFULFEATUREFUNCTION_H_

#include "FeatureFunction.h"
#include "FFState.h"
#include "../MemPool.h"

namespace Moses2
{

class Hypothesis;
class InputType;

namespace SCFG
{
class Hypothesis;
class Manager;
}

class StatefulFeatureFunction: public FeatureFunction
{
public:
  StatefulFeatureFunction(size_t startInd, const std::string &line);
  virtual ~StatefulFeatureFunction();

  void SetStatefulInd(size_t ind) {
    m_statefulInd = ind;
  }
  size_t GetStatefulInd() const {
    return m_statefulInd;
  }

  //! return uninitialise state
  virtual FFState* BlankState(MemPool &pool, const System &sys) const = 0;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual void EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                    const InputType &input, const Hypothesis &hypo) const = 0;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
                                   const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                   FFState &state) const = 0;

  virtual void EvaluateWhenApplied(const SCFG::Manager &mgr,
                                   const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                   FFState &state) const = 0;

  virtual void EvaluateWhenAppliedBatch(
    const System &system,
    const Batch &batch) const;

protected:
  size_t m_statefulInd;

};

}

#endif /* STATEFULFEATUREFUNCTION_H_ */
