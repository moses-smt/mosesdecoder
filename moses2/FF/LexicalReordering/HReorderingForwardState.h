/*
 * HReorderingForwardState.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#pragma once
#include "LRState.h"

namespace Moses2
{
class Range;
class Bitmap;
class InputPathBase;

class HReorderingForwardState: public LRState
{
public:
  HReorderingForwardState(const LRModel &config, size_t offset);
  virtual ~HReorderingForwardState();

  void Init(const LRState *prev, const TargetPhrase<Moses2::Word> &topt,
            const InputPathBase &path, bool first, const Bitmap *coverage);

  size_t hash() const;
  virtual bool operator==(const FFState& other) const;
  virtual std::string ToString() const;
  void Expand(const ManagerBase &mgr, const LexicalReordering &ff,
              const Hypothesis &hypo, size_t phraseTableInd, Scores &scores,
              FFState &state) const;

protected:
  bool m_first;
  //const Range &m_prevRange;
  const InputPathBase *prevPath;
  const Bitmap *m_coverage;

};

} /* namespace Moses2 */

