// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace Moses
{

class ExampleLM : public LanguageModelSingleFactor
{
protected:

public:
  ExampleLM(const std::string &line);
  ~ExampleLM();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
