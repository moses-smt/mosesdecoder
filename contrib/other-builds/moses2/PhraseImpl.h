#pragma once
#include "Phrase.h"

namespace Moses2
{
class SubPhrase;

class PhraseImpl : public Phrase
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab, const System &system, const std::string &str);

  PhraseImpl(MemPool &pool, size_t size);
  PhraseImpl(MemPool &pool, const PhraseImpl &copy);
  virtual ~PhraseImpl();

  const Word& operator[](size_t pos) const {
	return m_words[pos];
  }

  Word& operator[](size_t pos) {
	return m_words[pos];
  }

  size_t GetSize() const
  { return m_size; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const;

  void Prefetch() const;
protected:
  size_t m_size;
  Word *m_words;

  void CreateFromString(FactorCollection &vocab, const System &system, const std::vector<std::string> &toks);

};

}
