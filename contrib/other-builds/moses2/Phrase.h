/*
 * PhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <iostream>
#include "Word.h"
#include "MemPool.h"
#include "legacy/FactorCollection.h"

namespace Moses2
{

class SubPhrase;

class Phrase
{
	  friend std::ostream& operator<<(std::ostream &, const Phrase &);
public:
  virtual const Word& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;
  virtual size_t hash() const;
  virtual bool operator==(const Phrase &compare) const;
  virtual bool operator!=(const Phrase &compare) const
  {
		return !( (*this) == compare );
  }
  virtual std::string GetString(const FactorList &factorTypes) const;
  virtual SubPhrase GetSubPhrase(size_t start, size_t end) const = 0;

};

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

class SubPhrase : public Phrase
{
  friend std::ostream& operator<<(std::ostream &, const SubPhrase &);
public:
  SubPhrase(const PhraseImpl &origPhrase, size_t start, size_t end);
  virtual const Word& operator[](size_t pos) const
  { return (*m_origPhrase)[pos + m_start]; }

  virtual size_t GetSize() const
  { return m_end - m_start + 1; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const;

protected:
  const PhraseImpl *m_origPhrase;
  size_t m_start, m_end;
};

class PhraseOrdererLexical
{
public:
  bool operator()(const Phrase &a, const Phrase &b) const {
	size_t minSize = std::min(a.GetSize(), b.GetSize());
	for (size_t i = 0; i < minSize; ++i) {
		const Word &aWord = a[i];
		const Word &bWord = b[i];
		int cmp = aWord.Compare(bWord);
		//std::cerr << "WORD: " << aWord << " ||| " << bWord << " ||| " << lessThan << std::endl;
		if (cmp) {
			return (cmp < 0);
		}
	}
	return a.GetSize() < b.GetSize();
  }
};

}

