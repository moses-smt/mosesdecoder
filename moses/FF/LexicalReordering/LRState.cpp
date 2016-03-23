// -*- c++ -*-
#include <vector>
#include <string>

#include "LRState.h"
#include "moses/FF/FFState.h"
#include "moses/Hypothesis.h"
#include "moses/Range.h"
#include "moses/TranslationOption.h"
#include "moses/Util.h"

#include "LexicalReordering.h"

namespace Moses
{

void
LRState::
CopyScores(ScoreComponentCollection*  accum,
           const TranslationOption &topt,
           const InputType& input,
           ReorderingType reoType) const
{
  // don't call this on a bidirectional object
  UTIL_THROW_IF2(m_direction != LRModel::Backward &&
                 m_direction != LRModel::Forward,
                 "Unknown direction: " << m_direction);

  TranslationOption const* relevantOpt = ((m_direction == LRModel::Backward)
                                          ? &topt : m_prevOption);

  LexicalReordering* producer = m_configuration.GetScoreProducer();
  Scores const* cached = relevantOpt->GetLexReorderingScores(producer);

  // The approach here is bizarre! Why create a whole vector and do
  // vector addition (acumm->PlusEquals) to update a single value? - UG
  size_t off_remote = m_offset + reoType;
  size_t off_local  = m_configuration.CollapseScores() ? m_offset : off_remote;

  UTIL_THROW_IF2(off_local >= producer->GetNumScoreComponents(),
                 "offset out of vector bounds!");

  // look up applicable score from vectore of scores
  if(cached) {
    UTIL_THROW_IF2(off_remote >= cached->size(), "offset out of vector bounds!");
    Scores scores(producer->GetNumScoreComponents(),0);
    scores[off_local ] = (*cached)[off_remote];
    accum->PlusEquals(producer, scores);
  }

  // else: use default scores (if specified)
  else if (producer->GetHaveDefaultScores()) {
    Scores scores(producer->GetNumScoreComponents(),0);
    scores[off_local] = producer->GetDefaultScore(off_remote);
    accum->PlusEquals(m_configuration.GetScoreProducer(), scores);
  }
  // note: if no default score, no cost

  const SparseReordering* sparse = m_configuration.GetSparseReordering();
  if (sparse) sparse->CopyScores(*relevantOpt, m_prevOption, input, reoType,
                                   m_direction, accum);
}


int
LRState::
ComparePrevScores(const TranslationOption *other) const
{
  LexicalReordering* producer = m_configuration.GetScoreProducer();
  const Scores* myScores = m_prevOption->GetLexReorderingScores(producer);
  const Scores* yrScores = other->GetLexReorderingScores(producer);

  if(myScores == yrScores) return 0;

  // The pointers are NULL if a phrase pair isn't found in the reordering table.
  if(yrScores == NULL) return -1;
  if(myScores == NULL) return  1;

  size_t stop = m_offset + m_configuration.GetNumberOfTypes();
  for(size_t i = m_offset; i < stop; i++) {
    if((*myScores)[i] < (*yrScores)[i]) return -1;
    if((*myScores)[i] > (*yrScores)[i]) return  1;
  }
  return 0;
}

}

