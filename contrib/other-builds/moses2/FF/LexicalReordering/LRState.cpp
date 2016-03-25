/*
 * LRState.cpp
 *
 *  Created on: 22 Mar 2016
 *      Author: hieu
 */
#include "LRState.h"
#include "LexicalReordering.h"

namespace Moses2 {

LRState::LRState(const LRModel &config,
		LRModel::Direction dir,
		size_t offset)
:m_configuration(config)
,m_direction(dir)
,m_offset(offset)
{
}

int
LRState::
ComparePrevScores(const TargetPhrase *other) const
{
  LexicalReordering* producer = m_configuration.GetScoreProducer();
  size_t phraseTableInd = producer->GetPhraseTableInd();
  const SCORE *myScores = (const SCORE*) prevTP->ffData[phraseTableInd]; //producer->
  const SCORE *yrScores = (const SCORE*) other->ffData[phraseTableInd]; //producer->

  if(myScores == yrScores) return 0;

  // The pointers are NULL if a phrase pair isn't found in the reordering table.
  if(yrScores == NULL) return -1;
  if(myScores == NULL) return  1;

  size_t stop = m_offset + m_configuration.GetNumberOfTypes();
  for(size_t i = m_offset; i < stop; i++) {
    if((myScores)[i] < (yrScores)[i]) return -1;
    if((myScores)[i] > (yrScores)[i]) return  1;
  }
  return 0;
}

}

