/*
 * PhraseLR.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "PhraseBasedReorderingState.h"
#include "LexicalReordering.h"
#include "../../PhraseBased/Hypothesis.h"

namespace Moses2 {

PhraseBasedReorderingState::PhraseBasedReorderingState(const LRModel &config, LRModel::Direction dir)
:LRState(config, dir)
{
	  // uninitialised
}

void PhraseBasedReorderingState::Expand(const System &system,
		const LexicalReordering &ff,
		const Hypothesis &hypo,
		size_t phraseTableInd,
		Scores &scores,
		FFState &state) const
{
  const PhraseBasedReorderingState &prevStateCast = static_cast<const PhraseBasedReorderingState&>(*this);
  PhraseBasedReorderingState &stateCast = static_cast<PhraseBasedReorderingState&>(state);

  const Range &currRange = hypo.GetInputPath().range;
  stateCast.path = &hypo.GetInputPath();
  stateCast.targetPhrase = &hypo.GetTargetPhrase();

  // calc orientation
  size_t orientation;
  const Range *prevRange = &prevStateCast.path->range;
  assert(prevRange);
  if (prevRange->GetStartPos() == NOT_FOUND) {
	  orientation = GetOrientation(currRange);
  }
  else {
	  orientation = GetOrientation(*prevRange, currRange);
  }

  // backwards
  const TargetPhrase &target = hypo.GetTargetPhrase();

  const SCORE *values = (const SCORE *) target.ffData[phraseTableInd];
  if (values) {
	  scores.PlusEquals(system, ff, values[orientation], orientation);
  }

  // forwards
  if (prevRange->GetStartPos() != NOT_FOUND) {
	  const TargetPhrase &prevTarget = *prevStateCast.targetPhrase;
	  const SCORE *prevValues = (const SCORE *) prevTarget.ffData[phraseTableInd];

	  if (prevValues) {
		  scores.PlusEquals(system, ff, prevValues[orientation + 3], orientation + 3);
	  }
  }
}

size_t PhraseBasedReorderingState::GetOrientation(Range const& cur) const
{
  return (cur.GetStartPos() == 0) ? 0 : 2;
}

size_t PhraseBasedReorderingState::GetOrientation(Range const& prev, Range const& cur) const
{
  if (cur.GetStartPos() == prev.GetEndPos() + 1) {
	  // monotone
	  return 0;
  }
  else if (prev.GetStartPos() ==  cur.GetEndPos() + 1) {
	  // swap
	  return 1;
  }
  else {
	  // discontinuous
	  return 2;
  }
}

} /* namespace Moses2 */
