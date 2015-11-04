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
#include "moses/FactorCollection.h"

class SubPhrase;

class Phrase
{
	  friend std::ostream& operator<<(std::ostream &, const Phrase &);
public:
  virtual const Word& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;
};

class PhraseImpl : public Phrase
{
public:
  static PhraseImpl *CreateFromString(MemPool &pool, Moses::FactorCollection &vocab, const std::string &str);

  PhraseImpl(MemPool &pool, size_t size);
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
protected:
  size_t m_size;
  Word *m_words;

  void CreateFromString(Moses::FactorCollection &vocab, const std::vector<std::string> &toks);

};

class SubPhrase : public Phrase
{
  friend std::ostream& operator<<(std::ostream &, const SubPhrase &);
public:
  SubPhrase(const PhraseImpl &origPhrase, size_t start, size_t size);
  virtual const Word& operator[](size_t pos) const
  { return (*m_origPhrase)[pos + m_start]; }

  virtual size_t GetSize() const
  { return m_end - m_start + 1; }

protected:
  const PhraseImpl *m_origPhrase;
  size_t m_start, m_end;
};
