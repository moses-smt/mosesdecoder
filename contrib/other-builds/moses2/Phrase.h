/*
 * PhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <iostream>
#include "Word.h"
#include "MemPool.h"
#include "TypeDef.h"
#include "legacy/FactorCollection.h"

namespace Moses2
{

class SubPhrase;
class Scores;
class PhraseTable;
class MemPool;
class System;

class Phrase
{
  friend std::ostream& operator<<(std::ostream &, const Phrase &);
public:
  virtual ~Phrase()
  {
  }
  virtual const Word& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;
  virtual const Word& Front() const
  { return (*this)[0]; }
  virtual const Word& Back() const
  { return (*this)[GetSize() - 1]; }

  virtual size_t hash() const;
  virtual bool operator==(const Phrase &compare) const;
  virtual bool operator!=(const Phrase &compare) const
  {
    return !((*this) == compare);
  }
  virtual std::string GetString(const FactorList &factorTypes) const;
  virtual SubPhrase GetSubPhrase(size_t start, size_t size) const = 0;

  virtual void OutputToStream(std::ostream &out) const;

};
////////////////////////////////////////////////////////////////////////
class TargetPhrase: public Phrase
{
  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);

public:
  const PhraseTable &pt;
  mutable void **ffData;
  SCORE *scoreProperties;

  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system);

  Scores &GetScores()
  {
    return *m_scores;
  }

  const Scores &GetScores() const
  {
    return *m_scores;
  }

  SCORE GetFutureScore() const;

  void SetEstimatedScore(const SCORE &value)
  {
    m_estimatedScore = value;
  }

  SCORE *GetScoresProperty(int propertyInd) const;

protected:
  Scores *m_scores;
  SCORE m_estimatedScore;

};

//////////////////////////////////////////
struct CompareFutureScore
{
  bool operator()(const TargetPhrase *a, const TargetPhrase *b) const
  {
    return a->GetFutureScore() > b->GetFutureScore();
  }

  bool operator()(const TargetPhrase &a, const TargetPhrase &b) const
  {
    return a.GetFutureScore() > b.GetFutureScore();
  }
};

////////////////////////////////////////////////////////////////////////
class PhraseOrdererLexical
{
public:
  bool operator()(const Phrase &a, const Phrase &b) const
  {
    size_t minSize = std::min(a.GetSize(), b.GetSize());
    for (size_t i = 0; i < minSize; ++i) {
      const Word &aWord = a[i];
      const Word &bWord = b[i];
      int cmp = aWord.Compare(bWord);
      //std::cerr << "WORD: " << aWord << " ||| " << bWord << " ||| " << lessThan << std::endl;
      if (cmp) {
        return (cmp < 0);
      }
    }
    return a.GetSize() < b.GetSize();
  }
};

}

