#pragma once
#include "../PhraseImplTemplate.h"
#include "../SubPhrase.h"

namespace Moses2
{

class PhraseImpl: public PhraseImplTemplate<Word>
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab,
                                      const System &system, const std::string &str);

  PhraseImpl(MemPool &pool, size_t size) :
    PhraseImplTemplate<Word>(pool, size) {
  }

};

}
