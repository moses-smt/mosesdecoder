
#pragma once

#include <vector>
#include "Phrase.h"

class WordsRange;
class TargetPhrases;

struct PhraseTableLookup {
  const TargetPhrases *tpColl;
  const void *ptNode;

  void Set(const TargetPhrases *tpColl, const void *ptNode) {
    this->tpColl = tpColl;
    this->ptNode = ptNode;
  }
};

class InputPath
{
public:
  InputPath(const InputPath *prevPath, const Phrase *phrase, size_t endPos);
  virtual ~InputPath();

  const Phrase &GetPhrase() const {
    return *m_phrase;
  }

  const PhraseTableLookup &GetPtLookup(size_t ptId) const {
    return m_lookupColl[ptId];
  }
  PhraseTableLookup &GetPtLookup(size_t ptId) {
    return m_lookupColl[ptId];
  }

  const InputPath *GetPrevPath() const {
    return m_prevPath;
  }

  const WordsRange &GetRange() const {
    return *m_range;
  }

protected:
  const InputPath *m_prevPath;
  const Phrase *m_phrase;
  const WordsRange *m_range;
  std::vector<PhraseTableLookup> m_lookupColl; // arranged by pt
};

