#pragma once
#include <vector>
#include <string>
#include "Phrase.h"
#include "SubPhrase.h"
#include "legacy/Util2.h"

namespace Moses2
{
class SubPhrase;

class PhraseImpl : public Phrase
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, FactorCollection &vocab, const System &system, const std::string &str)
  {
	std::vector<std::string> toks = Moses2::Tokenize(str);
	size_t size = toks.size();
	PhraseImpl *ret;

	ret = new (pool.Allocate<PhraseImpl>()) PhraseImpl(pool, size);

	ret->CreateFromString(vocab, system, toks);
	return ret;
  }

  PhraseImpl(MemPool &pool, size_t size)
  :m_size(size)
  {
    m_words = new (pool.Allocate<Word>(size)) Word[size];

  }

  PhraseImpl(MemPool &pool, const PhraseImpl &copy)
  :m_size(copy.GetSize())
  {
    m_words = new (pool.Allocate<Word>(m_size)) Word[m_size];
  	for (size_t i = 0; i < m_size; ++i) {
  		const Word &word = copy[i];
  		(*this)[i] = word;
  	}
  }


  virtual ~PhraseImpl()
  {}

  const Word& operator[](size_t pos) const {
	return m_words[pos];
  }

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

  void Prefetch() const
  {
		for (size_t i = 0; i < m_size; ++i) {
			const Word &word = m_words[i];
			const Factor *factor = word[0];
			 __builtin_prefetch(factor);
		}
  }
protected:
  size_t m_size;
  Word *m_words;

  void CreateFromString(FactorCollection &vocab, const System &system, const std::vector<std::string> &toks)
  {
	for (size_t i = 0; i < m_size; ++i) {
		Word &word = (*this)[i];
		word.CreateFromString(vocab, system, toks[i]);
	}
  }

};


}
