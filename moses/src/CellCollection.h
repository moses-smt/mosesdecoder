// $Id: CellCollection.h 3048 2010-04-05 17:25:26Z hieuhoang1972 $
#pragma once

#include <vector>

namespace Moses
{
class Word;

class CellCollection
{
public:
  virtual ~CellCollection()
    {}
  virtual const std::vector<Word> &GetHeadwords(const Moses::WordsRange &coverage) const = 0;

};

}

