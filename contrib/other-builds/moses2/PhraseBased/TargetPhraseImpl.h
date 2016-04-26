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

class TargetPhraseImpl: public TargetPhrase<Moses2::Word>, public PhraseImplTemplate<Word>
{
  friend std::ostream& operator<<(std::ostream &, const TargetPhraseImpl &);
public:

  static TargetPhraseImpl *CreateFromString(MemPool &pool,
      const PhraseTable &pt, const System &system, const std::string &str);
  TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system,
      size_t size);
  //TargetPhraseImpl(MemPool &pool, const System &system, const TargetPhraseImpl &copy);

  virtual ~TargetPhraseImpl();

  const Word& operator[](size_t pos) const
  {
    return m_words[pos];
  }

  Word& operator[](size_t pos)
  {
    return m_words[pos];
  }

  size_t GetSize() const
  {
    return m_size;
  }

  SubPhrase<Moses2::Word> GetSubPhrase(size_t start, size_t end) const
  {
    SubPhrase<Moses2::Word> ret(*this, start, end);
    return ret;
  }

  //mutable void *chartState;

protected:
};

}

