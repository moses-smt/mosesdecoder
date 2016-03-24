#pragma once
#include "LRState.h"

namespace Moses
{
//! State for the standard Moses implementation of lexical reordering models
//! (see Koehn et al, Edinburgh System Description for the 2005 NIST MT
//! Evaluation)
class PhraseBasedReorderingState
  : public LRState
{
private:
  Range m_prevRange;
  bool m_first;
public:
  static bool m_useFirstBackwardScore;
  PhraseBasedReorderingState(const LRModel &config,
                             LRModel::Direction dir,
                             size_t offset);
  PhraseBasedReorderingState(const PhraseBasedReorderingState *prev,
                             const TranslationOption &topt);

  virtual size_t hash() const;
  virtual bool operator==(const FFState& other) const;

  virtual
  LRState*
  Expand(const TranslationOption& topt,const InputType& input,
         ScoreComponentCollection*  scores) const;
};

}

