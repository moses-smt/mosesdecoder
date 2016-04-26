#pragma once
#include "Phrase.h"

namespace Moses2
{

class SubPhrase: public Phrase<Word>
{
  friend std::ostream& operator<<(std::ostream &, const SubPhrase &);
public:
  SubPhrase(const Phrase<Word> &origPhrase, size_t start, size_t size)
  :m_origPhrase(&origPhrase)
  ,m_start(start)
  ,m_size(size)
  {}

  virtual const Word& operator[](size_t pos) const
  {  return (*m_origPhrase)[pos + m_start]; }

  virtual size_t GetSize() const
  {  return m_size; }

  SubPhrase GetSubPhrase(size_t start, size_t size) const
  {
    SubPhrase ret(*m_origPhrase, m_start + start, m_size);
    return ret;
  }

protected:
  const Phrase *m_origPhrase;
  size_t m_start, m_size;
};

///////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream &out, const SubPhrase &obj)
{
  if (obj.GetSize()) {
    out << obj[0];
    for (size_t i = 1; i < obj.GetSize(); ++i) {
      const Word &word = obj[i];
      out << " " << word;
    }
  }
  return out;
}

}

