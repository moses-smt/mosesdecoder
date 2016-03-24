/*
 * HReorderingBackwardState.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#pragma once
#include "LRState.h"

namespace Moses2 {

class HReorderingBackwardState : public LRState
{
public:
  HReorderingBackwardState(LRModel::Direction dir);
  virtual ~HReorderingBackwardState();

  size_t hash() const;
  virtual bool operator==(const FFState& other) const;
  virtual std::string ToString() const;
  void Expand(const System &system,
		  const LexicalReordering &ff,
		  const Hypothesis &hypo,
		  size_t phraseTableInd,
		  Scores &scores,
		  FFState &state) const;

};

} /* namespace Moses2 */

