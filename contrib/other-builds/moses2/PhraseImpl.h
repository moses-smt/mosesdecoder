#pragma once
#include "PhraseImplTemplate.h"
#include "SubPhrase.h"

namespace Moses2
{

class PhraseImpl : public Phrase, public PhraseImplTemplate<Word>
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab, const System &system, const std::string &str)
  {
	std::vector<std::string> toks = Moses2::Tokenize(str);
	size_t size = toks.size();
	PhraseImpl *ret;

	ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

	ret->PhraseImplTemplate<Word>::CreateFromString(vocab, system, toks);
	return ret;
  }

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

  SubPhrase GetSubPhrase(size_t start, size_t size) const
  {
	SubPhrase ret(*this, start, size);
	return ret;
  }

};


}
