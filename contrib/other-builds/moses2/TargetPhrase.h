/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "Phrase.h"
#include "PhraseImplTemplate.h"
#include "MemPool.h"
#include "Word.h"
#include "SubPhrase.h"

namespace Moses2
{

class Scores;
class Manager;
class System;
class PhraseTable;

class TargetPhrase : public TPBase, public PhraseImplTemplate<Word>
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:
  mutable void **ffData;
  SCORE *scoreProperties;

  static TargetPhrase *CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str);
  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size);
  //TargetPhrase(MemPool &pool, const System &system, const TargetPhrase &copy);

  virtual ~TargetPhrase();

  const Word& operator[](size_t pos) const
  {	return m_words[pos]; }

  Word& operator[](size_t pos)
  {	return m_words[pos]; }

  size_t GetSize() const
  { return m_size; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const
  {
	SubPhrase ret(*this, start, end);
	return ret;
  }

  SCORE *GetScoresProperty(int propertyInd) const;

  //mutable void *chartState;

protected:
};

//////////////////////////////////////////
struct CompareFutureScore {
  bool operator() (const TargetPhrase *a, const TargetPhrase *b) const
  {
	  return a->GetFutureScore() > b->GetFutureScore();
  }

  bool operator() (const TargetPhrase &a, const TargetPhrase &b) const
  {
	  return a.GetFutureScore() > b.GetFutureScore();
  }
};

}

