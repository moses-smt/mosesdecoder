/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "UnknownWordPenalty.h"
#include "../System.h"
#include "../Scores.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "../PhraseBased/InputPath.h"
#include "../PhraseBased/TargetPhrases.h"
#include "../SCFG/InputPath.h"
#include "../SCFG/TargetPhraseImpl.h"

using namespace std;

namespace Moses2
{

UnknownWordPenalty::UnknownWordPenalty(size_t startInd, const std::string &line) :
    PhraseTable(startInd, line)
{
  ReadParameters();
}

UnknownWordPenalty::~UnknownWordPenalty()
{
  // TODO Auto-generated destructor stub
}

void UnknownWordPenalty::Lookup(const Manager &mgr,
    InputPathsBase &inputPaths) const
{
  BOOST_FOREACH(InputPathBase *pathBase, inputPaths){
  InputPath *path = static_cast<InputPath*>(pathBase);
  const SubPhrase &phrase = path->subPhrase;

  TargetPhrases *tpsPtr;
  tpsPtr = Lookup(mgr, mgr.GetPool(), *path);
  path->AddTargetPhrases(*this, tpsPtr);
}

}

TargetPhrases *UnknownWordPenalty::Lookup(const Manager &mgr, MemPool &pool,
    InputPathBase &inputPath) const
{
  const System &system = mgr.system;

  TargetPhrases *tps = NULL;

  size_t numWords = inputPath.range.GetNumWordsCovered();
  if (numWords > 1) {
    // only create 1 word phrases
    return tps;
  }

  // any other pt translate this?
  size_t numPt = mgr.system.mappings.size();
  const TargetPhrases **allTPS =
      static_cast<InputPath&>(inputPath).targetPhrases;
  for (size_t i = 0; i < numPt; ++i) {
    const TargetPhrases *otherTps = allTPS[i];

    if (otherTps && otherTps->GetSize()) {
      return tps;
    }
  }

  const SubPhrase &source = inputPath.subPhrase;
  const Word &sourceWord = source[0];
  const Factor *factor = sourceWord[0];

  tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, 1);

  TargetPhraseImpl *target =
      new (pool.Allocate<TargetPhraseImpl>()) TargetPhraseImpl(pool, *this,
          system, 1);
  Word &word = (*target)[0];

  //FactorCollection &fc = system.vocab;
  //const Factor *factor = fc.AddFactor("SSS", false);
  word[0] = factor;

  Scores &scores = target->GetScores();
  scores.PlusEquals(mgr.system, *this, -100);

  MemPool &memPool = mgr.GetPool();
  system.featureFunctions.EvaluateInIsolation(memPool, system, source, *target);

  tps->AddTargetPhrase(*target);
  system.featureFunctions.EvaluateAfterTablePruning(memPool, *tps, source);

  return tps;
}

void UnknownWordPenalty::EvaluateInIsolation(const System &system,
    const Phrase &source, const TargetPhrase &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{

}

void UnknownWordPenalty::InitActiveChart(SCFG::InputPath &path) const
{
}

void UnknownWordPenalty::Lookup(MemPool &pool,
    const System &system,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const
{
  // terminal
  const Word &lastWord = path.subPhrase.Back();
  //cerr << "UnknownWordPenalty lastWord=" << lastWord << endl;

  if (path.range.GetNumWordsCovered() == 1) {
    const Factor *factor = lastWord[0];
    SCFG::TargetPhraseImpl *tp = new (pool.Allocate<SCFG::TargetPhraseImpl>(1)) SCFG::TargetPhraseImpl(pool, *this, system, 1);
    Word &word = (*tp)[0];
    word[0] = system.GetVocab().AddFactor(factor->GetString(), system, false);

    tp->lhs[0] = system.GetVocab().AddFactor("[X]", system, true);

    path.AddTargetPhrase(*this, tp);
  }
}
}

