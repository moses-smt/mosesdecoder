/*
 * WordPenalty.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "WordPenalty.h"
#include "../TypeDef.h"
#include "../Scores.h"
#include "../Phrase.h"
#include "../TargetPhrase.h"
#include "../SCFG/Word.h"
#include "../PhraseBased/TargetPhraseImpl.h"

namespace Moses2
{

WordPenalty::WordPenalty(size_t startInd, const std::string &line) :
  StatelessFeatureFunction(startInd, line)
{
  ReadParameters();
}

WordPenalty::~WordPenalty()
{
  // TODO Auto-generated destructor stub
}

void WordPenalty::EvaluateInIsolation(MemPool &pool, const System &system,
                                      const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
                                      SCORE &estimatedScore) const
{
  SCORE score = -(SCORE) targetPhrase.GetSize();
  scores.PlusEquals(system, *this, score);
}

void WordPenalty::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                      SCORE &estimatedScore) const
{
  size_t count = 0;
  for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
    const SCFG::Word &word = targetPhrase[i];
    if (!word.isNonTerminal) {
      ++count;
    }
  }
  scores.PlusEquals(system, *this, -(SCORE) count);
}

}

