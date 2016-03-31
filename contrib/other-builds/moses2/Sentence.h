/*
 * Sentence.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */
#pragma once

#include <string>
#include "InputType.h"
#include "PhraseImpl.h"

namespace Moses2
{
class FactorCollection;
class System;

class Sentence: public InputType, public PhraseImpl
{
public:
  static Sentence *CreateFromString(MemPool &pool, FactorCollection &vocab,
      const System &system, const std::string &str, long translationId,
      bool addBOSEOS);

  Sentence(long translationId, MemPool &pool, size_t size);
  virtual ~Sentence();
};

} /* namespace Moses2 */

