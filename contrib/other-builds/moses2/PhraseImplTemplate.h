/*
 * PhraseImplTemplate.h
 *
 *  Created on: 22 Feb 2016
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <string>
#include "Phrase.h"
#include "SubPhrase.h"
#include "legacy/Util2.h"



namespace Moses2
{
class SubPhrase;

template<typename WORD>
class PhraseImplTemplate : public Phrase
{
public:
  PhraseImplTemplate(MemPool &pool, size_t size)
  :m_size(size)
  {
    m_words = new (pool.Allocate<WORD>(size)) WORD[size];

  }

  PhraseImplTemplate(MemPool &pool, const PhraseImplTemplate &copy)
  :m_size(copy.GetSize())
  {
    m_words = new (pool.Allocate<WORD>(m_size)) WORD[m_size];
  	for (size_t i = 0; i < m_size; ++i) {
  		const WORD &word = copy[i];
  		(*this)[i] = word;
  	}
  }


  virtual ~PhraseImplTemplate()
  {}

  const WORD& operator[](size_t pos) const {
	return m_words[pos];
  }

  WORD& operator[](size_t pos) {
	return m_words[pos];
  }

  size_t GetSize() const
  { return m_size; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const
  {
	SubPhrase ret(*this, start, end);
	return ret;
  }

protected:
  size_t m_size;
  WORD *m_words;

  void CreateFromString(FactorCollection &vocab, const System &system, const std::vector<std::string> &toks)
  {
	for (size_t i = 0; i < m_size; ++i) {
		WORD &word = (*this)[i];
		word.CreateFromString(vocab, system, toks[i]);
	}
  }
};

}




