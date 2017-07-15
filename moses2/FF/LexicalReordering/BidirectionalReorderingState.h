/*
 * BidirectionalReorderingState.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#pragma once
#include "LRState.h"

namespace Moses2
{

class BidirectionalReorderingState: public LRState
{
public:
  BidirectionalReorderingState(const LRModel &config, LRState *bw, LRState *fw,
                               size_t offset);

  virtual ~BidirectionalReorderingState();

  void Init(const LRState *prev, const TargetPhrase<Moses2::Word> &topt,
            const InputPathBase &path, bool first, const Bitmap *coverage);

  size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual std::string ToString() const;

  void Expand(const ManagerBase &mgr, const LexicalReordering &ff,
              const Hypothesis &hypo, size_t phraseTableInd, Scores &scores,
              FFState &state) const;

protected:
  LRState *m_backward;
  LRState *m_forward;

};

} /* namespace Moses2 */

