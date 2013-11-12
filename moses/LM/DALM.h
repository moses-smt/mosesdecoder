// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace Moses
{

class LanguageModelDALM : public LanguageModelSingleFactor
{
protected:

public:
	LanguageModelDALM(const std::string &line);
  ~LanguageModelDALM();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
