/*
 * PhraseLR.h
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */

#pragma once
#include "LRState.h"

namespace Moses2
{

class InputPathBase;

class PhraseBasedReorderingState: public LRState
{
public:
  const InputPathBase *prevPath;
  bool m_first;

  PhraseBasedReorderingState(const LRModel &config, LRModel::Direction dir,
                             size_t offset);

  void Init(const LRState *prev, const TargetPhrase<Moses2::Word> &topt,
            const InputPathBase &path, bool first, const Bitmap *coverage);

  size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual std::string ToString() const {
    return "PhraseBasedReorderingState";
  }

  void Expand(const ManagerBase &mgr, const LexicalReordering &ff,
              const Hypothesis &hypo, size_t phraseTableInd, Scores &scores,
              FFState &state) const;

protected:

};

} /* namespace Moses2 */

