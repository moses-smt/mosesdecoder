#pragma once

#include "StackVec.h"

namespace Moses {

class TargetPhraseCollection;
class WordsRange;

class ChartParserCallback {
  public:
    virtual ~ChartParserCallback() {}

    virtual void Add(const TargetPhraseCollection &, const StackVec &, const WordsRange &) = 0;

    virtual bool Empty() const = 0;
};

} // namespace Moses
