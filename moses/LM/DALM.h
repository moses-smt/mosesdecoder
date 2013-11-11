// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace DALM
{
class Logger;
class Vocabulary;
class LM;
}

namespace Moses
{

class LanguageModelDALM : public LanguageModelSingleFactor
{
protected:
	std::string m_inifile;
	DALM::Logger *m_logger;
	DALM::Vocabulary *m_vocab;
	DALM::LM *m_lm;

public:
	LanguageModelDALM(const std::string &line);
  ~LanguageModelDALM();
  void Load();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
