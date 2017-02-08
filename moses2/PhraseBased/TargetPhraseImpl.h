/*
 * TargetPhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "../Phrase.h"
#include "../PhraseImplTemplate.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Word.h"
#include "../SubPhrase.h"

namespace Moses2
{

class Scores;
class Manager;
class System;
class PhraseTable;

class TargetPhraseImpl: public TargetPhrase<Moses2::Word>
{
public:
  typedef TargetPhrase<Moses2::Word> Parent;

  static TargetPhraseImpl *CreateFromString(MemPool &pool,
      const PhraseTable &pt, const System &system, const std::string &str);
  TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system,
                   size_t size);
  //TargetPhraseImpl(MemPool &pool, const System &system, const TargetPhraseImpl &copy);

  virtual ~TargetPhraseImpl();

  SCORE GetFutureScore() const {
    return m_scores->GetTotalScore() + m_estimatedScore;
  }

  void SetEstimatedScore(const SCORE &value) {
    m_estimatedScore = value;
  }

  virtual SCORE GetScoreForPruning() const {
    return GetFutureScore();
  }

protected:
  SCORE m_estimatedScore;

};

}

