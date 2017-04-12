/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */
#include <sstream>
#include "SkeletonStatefulFF.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/Hypothesis.h"

using namespace std;

namespace Moses2
{

class SkeletonState: public FFState
{
public:
  int targetLen;

  SkeletonState() {
    // uninitialised
  }

  virtual size_t hash() const {
    return (size_t) targetLen;
  }
  virtual bool operator==(const FFState& o) const {
    const SkeletonState& other = static_cast<const SkeletonState&>(o);
    return targetLen == other.targetLen;
  }

  virtual std::string ToString() const {
    stringstream sb;
    sb << targetLen;
    return sb.str();
  }

};

////////////////////////////////////////////////////////////////////////////////////////
SkeletonStatefulFF::SkeletonStatefulFF(size_t startInd, const std::string &line) :
  StatefulFeatureFunction(startInd, line)
{
  ReadParameters();
}

SkeletonStatefulFF::~SkeletonStatefulFF()
{
  // TODO Auto-generated destructor stub
}

FFState* SkeletonStatefulFF::BlankState(MemPool &pool, const System &sys) const
{
  return new (pool.Allocate<SkeletonState>()) SkeletonState();
}

void SkeletonStatefulFF::EmptyHypothesisState(FFState &state,
    const ManagerBase &mgr, const InputType &input,
    const Hypothesis &hypo) const
{
  SkeletonState &stateCast = static_cast<SkeletonState&>(state);
  stateCast.targetLen = 0;
}

void SkeletonStatefulFF::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void SkeletonStatefulFF::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void SkeletonStatefulFF::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  SkeletonState &stateCast = static_cast<SkeletonState&>(state);
  stateCast.targetLen = hypo.GetTargetPhrase().GetSize();
}

void SkeletonStatefulFF::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

