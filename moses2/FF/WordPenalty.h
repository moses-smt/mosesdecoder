/*
 * WordPenalty.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef WORDPENALTY_H_
#define WORDPENALTY_H_

#include "StatelessFeatureFunction.h"

namespace Moses2
{

class WordPenalty: public StatelessFeatureFunction
{
public:
  WordPenalty(size_t startInd, const std::string &line);
  virtual ~WordPenalty();

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
                      const TargetPhraseImpl &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                      SCORE &estimatedScore) const;

};

}

#endif /* WORDPENALTY_H_ */

