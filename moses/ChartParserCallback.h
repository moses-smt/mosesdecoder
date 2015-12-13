#pragma once

#include "StackVec.h"

#include <list>
#include "TargetPhraseCollection.h"

namespace Moses
{

class TargetPhraseCollection;
class Range;
class TargetPhrase;
class InputPath;
class InputType;
class ChartCellLabel;

class ChartParserCallback
{
public:
  virtual ~ChartParserCallback() {}

  virtual void Add(const TargetPhraseCollection &, const StackVec &, const Range &) = 0;

  virtual bool Empty() const = 0;

  virtual void AddPhraseOOV(TargetPhrase &phrase, std::list<TargetPhraseCollection::shared_ptr > &waste_memory, const Range &range) = 0;

  virtual void EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath) = 0;

  virtual float GetBestScore(const ChartCellLabel *chartCell) const = 0;

};

} // namespace Moses
