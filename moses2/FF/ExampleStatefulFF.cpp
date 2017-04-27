/*
 * ExampleStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */
#include <sstream>
#include "ExampleStatefulFF.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/Hypothesis.h"

using namespace std;

namespace Moses2
{

class ExampleState: public FFState
{
public:
  int targetLen;

  ExampleState() {
    // uninitialised
  }

  virtual size_t hash() const {
    return (size_t) targetLen;
  }
  virtual bool operator==(const FFState& o) const {
    const ExampleState& other = static_cast<const ExampleState&>(o);
    return targetLen == other.targetLen;
  }

  virtual std::string ToString() const {
    stringstream sb;
    sb << targetLen;
    return sb.str();
  }

};

////////////////////////////////////////////////////////////////////////////////////////
ExampleStatefulFF::ExampleStatefulFF(size_t startInd, const std::string &line) :
  StatefulFeatureFunction(startInd, line)
{
  ReadParameters();
}

ExampleStatefulFF::~ExampleStatefulFF()
{
  // TODO Auto-generated destructor stub
}

FFState* ExampleStatefulFF::BlankState(MemPool &pool, const System &sys) const
{
  return new (pool.Allocate<ExampleState>()) ExampleState();
}

void ExampleStatefulFF::EmptyHypothesisState(FFState &state,
    const ManagerBase &mgr, const InputType &input,
    const Hypothesis &hypo) const
{
  ExampleState &stateCast = static_cast<ExampleState&>(state);
  stateCast.targetLen = 0;
}

void ExampleStatefulFF::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void ExampleStatefulFF::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void ExampleStatefulFF::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  ExampleState &stateCast = static_cast<ExampleState&>(state);
  stateCast.targetLen = hypo.GetTargetPhrase().GetSize();
}

void ExampleStatefulFF::EvaluateWhenApplied(const SCFG::Manager &mgr,
    const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

