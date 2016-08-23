// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace Moses
{

class SkeletonLM : public LanguageModelSingleFactor
{
protected:

public:
  SkeletonLM(const std::string &line);
  ~SkeletonLM();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
