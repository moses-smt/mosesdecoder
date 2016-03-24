/*
 * HReorderingBackwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingBackwardState.h"

namespace Moses2 {

HReorderingBackwardState::HReorderingBackwardState(const LRModel &config,
		LRModel::Direction dir)
:LRState(config, dir)
{
	// TODO Auto-generated constructor stub

}

HReorderingBackwardState::~HReorderingBackwardState() {
	// TODO Auto-generated destructor stub
}

size_t HReorderingBackwardState::hash() const
{

}

bool HReorderingBackwardState::operator==(const FFState& other) const
{

}

std::string HReorderingBackwardState::ToString() const
{

}

void HReorderingBackwardState::Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{

}

} /* namespace Moses2 */
