#pragma once
#include "../PhraseImplTemplate.h"
#include "../SubPhrase.h"
#include "Word.h"

namespace Moses2
{
namespace SCFG
{

class PhraseImpl: public PhraseImplTemplate<SCFG::Word>
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab,
                                      const System &system, const std::string &str, bool skipLastWord = true);

  PhraseImpl(MemPool &pool, size_t size) :
    PhraseImplTemplate<Word>(pool, size) {
  }

};

}
}

