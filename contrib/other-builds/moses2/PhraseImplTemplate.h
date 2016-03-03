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
#include "legacy/Util2.h"



namespace Moses2
{
class SubPhrase;

template<typename WORD>
class PhraseImplTemplate
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

  size_t GetSize() const
  { return m_size; }


protected:
  size_t m_size;
  WORD *m_words;

  WORD& operator[](size_t pos) {
	return m_words[pos];
  }

  void CreateFromString(FactorCollection &vocab, const System &system, const std::vector<std::string> &toks, bool addBOSEOS = false)
  {
	for (size_t i = 0; i < m_size; ++i) {
		WORD &word = (*this)[i];
		word.CreateFromString(vocab, system, toks[i]);
	}
  }
};

}




