#pragma once
#include "../../legacy/FFState.h"
#include "LRModel.h"

namespace Moses2 {

class LexicalReordering;
class Hypothesis;
class System;
class Scores;
class Bitmap;
class ManagerBase;
class TargetPhrase;
class InputType;
class InputPathBase;

class LRState : public FFState
{
public:
  typedef LRModel::ReorderingType ReorderingType;
  const TargetPhrase *prevTP;

  LRState(const LRModel &config,
		  LRModel::Direction dir,
		  size_t offset);

  virtual void Init(const LRState *prev,
		  const TargetPhrase &topt,
		  const InputPathBase &path,
		  bool first,
		  const Bitmap *coverage) = 0;

  virtual void Expand(const ManagerBase &mgr,
			  const LexicalReordering &ff,
			  const Hypothesis &hypo,
			  size_t phraseTableInd,
			  Scores &scores,
			  FFState &state) const = 0;

  void CopyScores(const System &system,
		  Scores &accum,
          const TargetPhrase &topt,
          ReorderingType reoType) const;

protected:
  const LRModel& m_configuration;
  LRModel::Direction m_direction;
  size_t m_offset;

  int
  ComparePrevScores(const TargetPhrase *other) const;

};

}
