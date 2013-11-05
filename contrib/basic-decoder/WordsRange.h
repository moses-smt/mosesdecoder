
#pragma once

#include <unistd.h>
#include "TypeDef.h"

class WordsRange
{
public:
  WordsRange(); // do not implement

  WordsRange(const WordsRange &prevRange, size_t phraseSize);

  WordsRange(size_t s, size_t e)
    :startPos(s)
    ,endPos(e)
  {}

  virtual ~WordsRange();

  size_t startPos, endPos;

  //! count of words translated
  inline size_t GetNumWordsCovered() const {
    return (startPos == NOT_FOUND) ? 0 : endPos - startPos + 1;
  }

  inline size_t GetNumWordsBetween(const WordsRange &other) const {
    //CHECK(!Overlap(x));

    if (other.endPos < startPos) {
      return startPos - other.endPos - 1;
    }

    return other.startPos - endPos - 1;
  }

  int ComputeDistortionScore(const WordsRange &next) const;

  size_t GetHash() const;
  bool operator==(const WordsRange &other) const;

  std::string Debug() const;

};

