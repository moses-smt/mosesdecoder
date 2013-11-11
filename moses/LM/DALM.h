// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace Moses
{

class LanguageModelDALM : public LanguageModelSingleFactor
{
protected:
	std::string m_inifile;

public:
	LanguageModelDALM(const std::string &line);
  ~LanguageModelDALM();
  void Load();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
