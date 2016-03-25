#pragma once
#include "../../legacy/FFState.h"
#include "LRModel.h"

namespace Moses2 {

class LexicalReordering;
class Hypothesis;
class System;
class Scores;
class TargetPhrase;

class LRState : public FFState
{
public:
  const TargetPhrase *prevTP;

  LRState(const LRModel &config,
		  LRModel::Direction dir,
		  size_t offset);

  virtual void Expand(const System &system,
			  const LexicalReordering &ff,
			  const Hypothesis &hypo,
			  size_t phraseTableInd,
			  Scores &scores,
			  FFState &state) const = 0;
protected:
  const LRModel& m_configuration;
  LRModel::Direction m_direction;
  size_t m_offset;

  int
  ComparePrevScores(const TargetPhrase *other) const;

};

}
