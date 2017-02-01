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

template<typename WORD>
class PhraseImplTemplate : public Phrase<WORD>
{
public:
  PhraseImplTemplate(MemPool &pool, size_t size) :
    m_size(size) {
    m_words = new (pool.Allocate<WORD>(size)) WORD[size];

  }

  PhraseImplTemplate(MemPool &pool, const PhraseImplTemplate &copy) :
    m_size(copy.GetSize()) {
    m_words = new (pool.Allocate<WORD>(m_size)) WORD[m_size];
    for (size_t i = 0; i < m_size; ++i) {
      const WORD &word = copy[i];
      (*this)[i] = word;
    }
  }

  virtual ~PhraseImplTemplate() {
  }

  size_t GetSize() const {
    return m_size;
  }

  WORD& operator[](size_t pos) {
    return m_words[pos];
  }

  const WORD& operator[](size_t pos) const {
    return m_words[pos];
  }

  SubPhrase<WORD> GetSubPhrase(size_t start, size_t size) const {
    SubPhrase<WORD> ret(*this, start, size);
    return ret;
  }

protected:
  size_t m_size;
  WORD *m_words;

  void CreateFromString(FactorCollection &vocab, const System &system,
                        const std::vector<std::string> &toks, bool addBOSEOS = false) {
    size_t startPos = 0;
    if (addBOSEOS) {
      startPos = 1;

      m_words[0].CreateFromString(vocab, system, "<s>");
      m_words[m_size-1].CreateFromString(vocab, system, "</s>");
    }

    for (size_t i = 0; i < toks.size(); ++i) {
      WORD &word = (*this)[startPos];
      word.CreateFromString(vocab, system, toks[i]);
      ++startPos;
    }
  }
};

}

