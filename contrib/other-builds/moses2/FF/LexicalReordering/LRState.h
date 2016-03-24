#pragma once
#include "../../legacy/FFState.h"
#include "LRModel.h"

namespace Moses2 {

class LexicalReordering;
class Hypothesis;
class System;
class Scores;

class LRState : public FFState
{
public:
  LRState(LRModel::Direction dir);

  virtual void Expand(const System &system,
			  const LexicalReordering &ff,
			  const Hypothesis &hypo,
			  size_t phraseTableInd,
			  Scores &scores,
			  FFState &state) const = 0;
protected:
  LRModel::Direction m_direction;

};

}
