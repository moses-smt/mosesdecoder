/*
 * HReorderingBackwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingBackwardState.h"
#include "../../PhraseBased/Hypothesis.h"
#include "../../PhraseBased/Manager.h"

namespace Moses2
{

HReorderingBackwardState::HReorderingBackwardState(MemPool &pool,
    const LRModel &config, size_t offset) :
  LRState(config, LRModel::Backward, offset), reoStack(pool)
{
  // TODO Auto-generated constructor stub

}

HReorderingBackwardState::~HReorderingBackwardState()
{
  // TODO Auto-generated destructor stub
}

void HReorderingBackwardState::Init(const LRState *prev,
                                    const TargetPhrase<Moses2::Word> &topt, const InputPathBase &path, bool first,
                                    const Bitmap *coverage)
{
  prevTP = &topt;
  reoStack.Init();
}

size_t HReorderingBackwardState::hash() const
{
  size_t ret = reoStack.hash();
  return ret;
}

bool HReorderingBackwardState::operator==(const FFState& o) const
{
  const HReorderingBackwardState& other =
    static_cast<const HReorderingBackwardState&>(o);
  bool ret = reoStack == other.reoStack;
  return ret;
}

std::string HReorderingBackwardState::ToString() const
{
  return "HReorderingBackwardState " + SPrint(m_offset);
}

void HReorderingBackwardState::Expand(const ManagerBase &mgr,
                                      const LexicalReordering &ff, const Hypothesis &hypo, size_t phraseTableInd,
                                      Scores &scores, FFState &state) const
{
  HReorderingBackwardState &nextState =
    static_cast<HReorderingBackwardState&>(state);
  nextState.Init(this, hypo.GetTargetPhrase(), hypo.GetInputPath(), false,
                 NULL);
  nextState.reoStack = reoStack;

  const Range &swrange = hypo.GetInputPath().range;
  int reoDistance = nextState.reoStack.ShiftReduce(swrange);
  ReorderingType reoType = m_configuration.GetOrientation(reoDistance);
  CopyScores(mgr.system, scores, hypo.GetTargetPhrase(), reoType);
}

} /* namespace Moses2 */
