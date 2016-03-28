/*
 * HReorderingBackwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingBackwardState.h"
#include "../../PhraseBased/Hypothesis.h"

namespace Moses2 {

HReorderingBackwardState::HReorderingBackwardState(const LRModel &config,
		size_t offset)
:LRState(config, LRModel::Backward, offset)
{
	// TODO Auto-generated constructor stub

}

HReorderingBackwardState::~HReorderingBackwardState() {
	// TODO Auto-generated destructor stub
}

void HReorderingBackwardState::Init(const LRState *prev,
    const TargetPhrase &topt,
    const InputPathBase &path,
    bool first)
{
  prevTP = &topt;
}

size_t HReorderingBackwardState::hash() const
{
  size_t ret = reoStack.hash();
  return ret;
}

bool HReorderingBackwardState::operator==(const FFState& o) const
{
  const HReorderingBackwardState& other
  = static_cast<const HReorderingBackwardState&>(o);
  bool ret = reoStack == other.reoStack;
  return ret;
}

std::string HReorderingBackwardState::ToString() const
{
  return "HReorderingBackwardState";
}

void HReorderingBackwardState::Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{
  HReorderingBackwardState &nextState = static_cast<HReorderingBackwardState&>(state);
  nextState.Init(this, hypo.GetTargetPhrase(), hypo.GetInputPath(), false);
  nextState.reoStack = reoStack;

  const Range &swrange = hypo.GetInputPath().range;
  int reoDistance = nextState.reoStack.ShiftReduce(swrange);
  ReorderingType reoType = m_configuration.GetOrientation(reoDistance);
  CopyScores(system, scores, hypo.GetTargetPhrase(), reoType);
}

} /* namespace Moses2 */
