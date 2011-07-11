#pragma once

#include <vector>
#include "Word.h"

namespace Moses
{
class Word;

class CellCollection
{
public:
  virtual ~CellCollection()
    {}
  virtual const std::vector<const Moses::Word*> GetTargetLHSList(const Moses::WordsRange &coverage) const = 0;

};

}

