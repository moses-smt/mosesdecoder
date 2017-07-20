#pragma once
#include "../FFState.h"
#include "LRModel.h"

namespace Moses2
{
template<typename WORD>
class TargetPhrase;

class LexicalReordering;
class Hypothesis;
class System;
class Scores;
class Bitmap;
class ManagerBase;
class InputType;
class InputPathBase;
class Word;

class LRState: public FFState
{
public:
  typedef LRModel::ReorderingType ReorderingType;
  const TargetPhrase<Moses2::Word> *prevTP;

  LRState(const LRModel &config, LRModel::Direction dir, size_t offset);

  virtual void Init(const LRState *prev, const TargetPhrase<Moses2::Word> &topt,
                    const InputPathBase &path, bool first, const Bitmap *coverage) = 0;

  virtual void Expand(const ManagerBase &mgr, const LexicalReordering &ff,
                      const Hypothesis &hypo, size_t phraseTableInd, Scores &scores,
                      FFState &state) const = 0;

  void CopyScores(const System &system, Scores &accum, const TargetPhrase<Moses2::Word> &topt,
                  ReorderingType reoType) const;

protected:
  const LRModel& m_configuration;
  LRModel::Direction m_direction;
  size_t m_offset;

  int
  ComparePrevScores(const TargetPhrase<Moses2::Word> *other) const;

};

}
