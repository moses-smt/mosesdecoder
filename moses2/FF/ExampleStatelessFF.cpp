/*
 * SkeletonStatefulFF.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */
#include "../Scores.h"

#include "ExampleStatelessFF.h"

namespace Moses2
{

ExampleStatelessFF::ExampleStatelessFF(size_t startInd,
                                       const std::string &line) :
  StatelessFeatureFunction(startInd, line)
{
  ReadParameters();
}

ExampleStatelessFF::~ExampleStatelessFF()
{
  // TODO Auto-generated destructor stub
}

void ExampleStatelessFF::EvaluateInIsolation(MemPool &pool,
    const System &system, const Phrase<Moses2::Word> &source,
    const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

void ExampleStatelessFF::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
}

}

