/*
 * HReorderingForwardState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#include "HReorderingForwardState.h"

namespace Moses2 {

HReorderingForwardState::HReorderingForwardState(LRModel::Direction dir)
:LRState(dir)
{
	// TODO Auto-generated constructor stub

}

HReorderingForwardState::~HReorderingForwardState() {
	// TODO Auto-generated destructor stub
}

size_t HReorderingForwardState::hash() const
{

}

bool HReorderingForwardState::operator==(const FFState& other) const
{

}

std::string HReorderingForwardState::ToString() const
{

}

void HReorderingForwardState::Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const
{

}

} /* namespace Moses2 */
