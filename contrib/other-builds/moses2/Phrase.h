/*
 * Phrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include "Word.h"

class PhraseBase
{
public:
  virtual const Word& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;
};

class SubPhrase;

class Phrase : public PhraseBase
{
public:
  static Phrase *CreateFromString(const std::string &str);
  Phrase(size_t size);
  virtual ~Phrase();

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
};

class SubPhrase : public PhraseBase
{
public:
  SubPhrase(const Phrase &origPhrase, size_t start, size_t size);
  virtual const Word& operator[](size_t pos) const
  { return m_origPhrase[pos + m_start]; }

  virtual size_t GetSize() const
  { return m_end - m_start + 1; }

protected:
  size_t m_start, m_end;
  const Phrase &m_origPhrase;
};
