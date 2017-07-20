/*
 * LRState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include "LRState.h"
#include "LexicalReordering.h"
#include "../../Scores.h"
#include "../../TargetPhrase.h"

using namespace std;

namespace Moses2
{

class InputType;

LRState::LRState(const LRModel &config, LRModel::Direction dir, size_t offset) :
  m_configuration(config), m_direction(dir), m_offset(offset)
{
}

int LRState::ComparePrevScores(const TargetPhrase<Moses2::Word> *other) const
{
  LexicalReordering* producer = m_configuration.GetScoreProducer();
  size_t phraseTableInd = producer->GetPhraseTableInd();
  const SCORE *myScores = (const SCORE*) prevTP->ffData[phraseTableInd]; //producer->
  const SCORE *yrScores = (const SCORE*) other->ffData[phraseTableInd]; //producer->

  if (myScores == yrScores) return 0;

  // The pointers are NULL if a phrase pair isn't found in the reordering table.
  if (yrScores == NULL) return -1;
  if (myScores == NULL) return 1;

  size_t stop = m_offset + m_configuration.GetNumberOfTypes();
  for (size_t i = m_offset; i < stop; i++) {
    if ((myScores)[i] < (yrScores)[i]) return -1;
    if ((myScores)[i] > (yrScores)[i]) return 1;
  }
  return 0;
}

void LRState::CopyScores(const System &system, Scores &accum,
                         const TargetPhrase<Moses2::Word> &topt, ReorderingType reoType) const
{
  // don't call this on a bidirectional object
  UTIL_THROW_IF2(
    m_direction != LRModel::Backward && m_direction != LRModel::Forward,
    "Unknown direction: " << m_direction);

  TargetPhrase<Moses2::Word> const* relevantOpt = (
        (m_direction == LRModel::Backward) ? &topt : prevTP);

  LexicalReordering* producer = m_configuration.GetScoreProducer();
  size_t phraseTableInd = producer->GetPhraseTableInd();
  const SCORE *cached = (const SCORE*) relevantOpt->ffData[phraseTableInd]; //producer->

  if (cached == NULL) {
    return;
  }

  size_t off_remote = m_offset + reoType;
  size_t off_local = m_configuration.CollapseScores() ? m_offset : off_remote;

  UTIL_THROW_IF2(off_local >= producer->GetNumScores(),
                 "offset out of vector bounds!");

  // look up applicable score from vector of scores
  //UTIL_THROW_IF2(off_remote >= cached->size(), "offset out of vector bounds!");
  //Scores scores(producer->GetNumScoreComponents(),0);
  SCORE score = cached[off_remote];
  accum.PlusEquals(system, *producer, score, off_local);

  // else: use default scores (if specified)
  /*
   else if (producer->GetHaveDefaultScores()) {
   Scores scores(producer->GetNumScoreComponents(),0);
   scores[off_local] = producer->GetDefaultScore(off_remote);
   accum->PlusEquals(m_configuration.GetScoreProducer(), scores);
   }
   */
  // note: if no default score, no cost
  /*
   const SparseReordering* sparse = m_configuration.GetSparseReordering();
   if (sparse) sparse->CopyScores(*relevantOpt, m_prevOption, input, reoType,
   m_direction, accum);
   */
}

}

