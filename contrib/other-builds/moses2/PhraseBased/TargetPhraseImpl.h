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

  static TargetPhraseImpl *CreateFromString(MemPool &pool,
      const PhraseTable &pt, const System &system, const std::string &str);
  TargetPhraseImpl(MemPool &pool, const PhraseTable &pt, const System &system,
      size_t size);
  //TargetPhraseImpl(MemPool &pool, const System &system, const TargetPhraseImpl &copy);

  virtual ~TargetPhraseImpl();

  virtual void Debug(std::ostream &out, const System &system) const;

protected:
};

}

