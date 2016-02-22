#pragma once
#include "Phrase.h"

namespace Moses2
{

class SubPhrase : public Phrase
{
  friend std::ostream& operator<<(std::ostream &, const SubPhrase &);
public:
  SubPhrase(const Phrase &origPhrase, size_t start, size_t end);
  virtual const Word& operator[](size_t pos) const;

  virtual size_t GetSize() const
  { return m_end - m_start + 1; }

  SubPhrase GetSubPhrase(size_t start, size_t end) const;

protected:
  const Phrase *m_origPhrase;
  size_t m_start, m_end;
};

}

