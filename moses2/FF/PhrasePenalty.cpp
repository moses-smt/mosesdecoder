/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include "PhrasePenalty.h"
#include "../Scores.h"

namespace Moses2
{

PhrasePenalty::PhrasePenalty(size_t startInd, const std::string &line) :
  StatelessFeatureFunction(startInd, line)
{
  ReadParameters();
}

PhrasePenalty::~PhrasePenalty()
{
  // TODO Auto-generated destructor stub
}

void PhrasePenalty::EvaluateInIsolation(MemPool &pool, const System &system,
                                        const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
                                        SCORE &estimatedScore) const
{
  scores.PlusEquals(system, *this, 1);
}

void PhrasePenalty::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                        const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                        SCORE &estimatedScore) const
{
  scores.PlusEquals(system, *this, 1);
}

}

