#pragma once
#include "../PhraseImplTemplate.h"
#include "../SubPhrase.h"

namespace Moses2
{

class PhraseImpl: public PhraseImplTemplate<Word>
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab,
      const System &system, const std::string &str)
  {
    std::vector<std::string> toks = Moses2::Tokenize(str);
    size_t size = toks.size();
    PhraseImpl *ret;

    ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

    ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks);
    return ret;
  }

  PhraseImpl(MemPool &pool, size_t size) :
      PhraseImplTemplate<Word>(pool, size)
  {
  }

};

}
