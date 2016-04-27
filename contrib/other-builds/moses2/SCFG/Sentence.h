/*
 * Sentence.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#pragma once

#include <string>
#include "PhraseImpl.h"
#include "../InputType.h"
#include "../MemPool.h"
#include "../legacy/Util2.h"

namespace Moses2
{
class FactorCollection;
class System;

namespace SCFG
{

class Sentence: public InputType, public PhraseImpl
{
public:
  static Sentence *CreateFromString(MemPool &pool, FactorCollection &vocab,
      const System &system, const std::string &str, long translationId)
  {
    std::vector<std::string> toks = Tokenize(str);
    size_t size = toks.size();
    size += 2;

    Sentence *ret;

    ret = new (pool.Allocate<Sentence>()) Sentence(translationId, pool, size);

    ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks, true);

    return ret;
  }

  Sentence(long translationId, MemPool &pool, size_t size)
  :InputType(translationId)
  ,PhraseImpl(pool, size)
  {}

  virtual ~Sentence()
  {}

};

}
} /* namespace Moses2 */

