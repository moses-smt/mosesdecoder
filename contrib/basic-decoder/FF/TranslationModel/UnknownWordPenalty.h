/*
 * UnknownWordPenalty.h
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */
#pragma once

#include "PhraseTable.h"

class TargetPhrases;

class UnknownWordPenalty: public PhraseTable
{
public:
  UnknownWordPenalty(const std::string line);
  virtual ~UnknownWordPenalty();

  void CleanUpAfterSentenceProcessing(const Sentence &source);

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const {
  }

  void Lookup(const std::vector<InputPath*> &inputPathQueue);

protected:
  std::vector<TargetPhrases*> m_targetPhrases;
};

