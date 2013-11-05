
#pragma once

#include <vector>
#include <string>
#include <cassert>
#include "Word.h"

typedef std::vector<const Word*> PhraseVec;

class Phrase
{
public:
  static Phrase *CreateFromString(const std::string &line);

  Phrase(const Phrase &copy); // do not implement
  Phrase(size_t size);
  Phrase(const Phrase &copy, size_t extra);

  virtual ~Phrase();

  const Word &GetWord(size_t pos) const {
    return m_words[pos];
  }
  Word &GetWord(size_t pos) {
    assert(pos < m_size);
    return m_words[pos];
  }
  const Word &Back() const {
    assert(m_size);
    return m_words[m_size - 1];
  }

  size_t GetSize() const {
    return m_size;
  }

  void Set(size_t pos, const Word &word);
  void SetLastWord(const Word &word) {
    Set(m_size - 1, word);
  }

  void Output(std::ostream &out) const;
  virtual std::string Debug() const;

protected:
  size_t m_size;
  std::vector<Word> m_words;
};

