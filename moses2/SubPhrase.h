#pragma once
#include <sstream>
#include "Phrase.h"
#include "Word.h"
#include "SCFG/Word.h"

namespace Moses2
{
class System;

template<typename WORD>
class SubPhrase: public Phrase<WORD>
{
public:
  SubPhrase(const Phrase<WORD> &origPhrase, size_t start, size_t size)
    :m_origPhrase(&origPhrase)
    ,m_start(start)
    ,m_size(size)
  {}

  virtual const WORD& operator[](size_t pos) const {
    return (*m_origPhrase)[pos + m_start];
  }

  virtual size_t GetSize() const {
    return m_size;
  }

  SubPhrase GetSubPhrase(size_t start, size_t size) const {
    SubPhrase ret(*m_origPhrase, m_start + start, size);
    return ret;
  }

  virtual std::string Debug(const System &system) const {
    std::stringstream out;
    if (GetSize()) {
      out << (*this)[0].Debug(system);
      for (size_t i = 1; i < GetSize(); ++i) {
        const WORD &word = (*this)[i];
        out << " " << word.Debug(system);
      }
    }

    return out.str();
  }

protected:
  const Phrase<WORD> *m_origPhrase;
  size_t m_start, m_size;
};


}

