#pragma once
#include "../PhraseImplTemplate.h"
#include "../SubPhrase.h"
#include "Word.h"

namespace Moses2
{
namespace SCFG
{

class PhraseImpl : public Phrase, public PhraseImplTemplate<SCFG::Word>
{
public:
  SCFG::Word lhs;

  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab, const System &system, const std::string &str);

  PhraseImpl(MemPool &pool, size_t size)
  :PhraseImplTemplate(pool, size)
  {}

  const Word& operator[](size_t pos) const
  {	return m_words[pos]; }

  Word& operator[](size_t pos) {
	return m_words[pos];
  }

  size_t GetSize() const
  { return m_size; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const
  {
	SubPhrase ret(*this, start, end);
	return ret;
  }

};

}
}

