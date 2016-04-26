#pragma once
#include "../PhraseImplTemplate.h"
#include "../SubPhrase.h"
#include "Word.h"

namespace Moses2
{
namespace SCFG
{

class PhraseImpl: public Phrase<SCFG::Word>, public PhraseImplTemplate<SCFG::Word>
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab,
      const System &system, const std::string &str);

  PhraseImpl(MemPool &pool, size_t size) :
      PhraseImplTemplate<Word>(pool, size)
  {
  }

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

  SubPhrase<SCFG::Word> GetSubPhrase(size_t start, size_t size) const
  {
    SubPhrase<SCFG::Word> ret(*this, start, size);
    return ret;
  }

};

}
}

