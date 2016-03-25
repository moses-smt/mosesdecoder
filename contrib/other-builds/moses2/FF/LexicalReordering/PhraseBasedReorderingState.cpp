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

PhraseBasedReorderingState::PhraseBasedReorderingState(
		const LRModel &config,
		LRModel::Direction dir,
		size_t offset)
:LRState(config, dir, offset)
{
	  // uninitialised
}

size_t PhraseBasedReorderingState::hash() const {
	  size_t ret;
	  ret = hash_value(prevPath->range);
	  boost::hash_combine(ret, m_direction);

	  return ret;
}

bool PhraseBasedReorderingState::operator==(const FFState& o) const {
	  if (&o == this) return true;

	  const PhraseBasedReorderingState &other = static_cast<const PhraseBasedReorderingState&>(o);
	  if (prevPath->range == other.prevPath->range) {
	    if (m_direction == LRModel::Forward) {
	      int compareScore = ComparePrevScores(other.prevTP);
	      return compareScore == 0;
	    } else {
	      return true;
	    }
	  } else {
	    return false;
	  }
}

void PhraseBasedReorderingState::Expand(const System &system,
		const LexicalReordering &ff,
		const Hypothesis &hypo,
		size_t phraseTableInd,
		Scores &scores,
		FFState &state) const
{
	  // const LRModel::ModelType modelType = m_configuration.GetModelType();

	  if ((m_direction != LRModel::Forward) || !m_first) {
	    LRModel const& lrmodel = m_configuration;
	    Range const &cur = hypo.GetInputPath().range;
	    LRModel::ReorderingType reoType = (m_first ? lrmodel.GetOrientation(cur)
	                                       : lrmodel.GetOrientation(prevPath->range, cur));
	    //CopyScores(scores, topt, input, reoType);
	  }
//	  return new PhraseBasedReorderingState(this, topt);

}

} /* namespace Moses2 */
