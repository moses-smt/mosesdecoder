/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */
#include "../Scores.h"

#include "SkeletonStatelessFF.h"

namespace Moses2
{

SkeletonStatelessFF::SkeletonStatelessFF(size_t startInd,
    const std::string &line) :
  StatelessFeatureFunction(startInd, line)
{
  ReadParameters();
}

SkeletonStatelessFF::~SkeletonStatelessFF()
{
  // TODO Auto-generated destructor stub
}

void SkeletonStatelessFF::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void SkeletonStatelessFF::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

}

