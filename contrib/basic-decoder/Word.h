
#pragma once

#include <string>
#include "TypeDef.h"

class Word
{
public:
  Word();
  virtual ~Word();

  void CreateFromString(const std::string &line);

  VOCABID GetVocab() const {
    return m_vocabId;
  }

  void Set(const Word &word);

  void Output(std::ostream &out) const;
  std::string ToString() const;

  std::string Debug() const;

  int Compare(const Word &other) const {
    if (m_vocabId == other.m_vocabId) {
      return 0;
    }

    return (m_vocabId < other.m_vocabId) ? -1 : +1;
  }

  bool operator== (const Word &other) const {
    // needed to store word in GenerationDictionary map
    // uses comparison of FactorKey
    // 'proper' comparison, not address/id comparison
    return Compare(other) == 0;
  }

protected:
  VOCABID m_vocabId;
};

class WordHasher
{
public:
  size_t operator()(const Word &word) const {
    return word.GetVocab();
  }
};

