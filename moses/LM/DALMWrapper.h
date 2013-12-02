// $Id$
#pragma once

#include <vector>
#include <boost/bimap.hpp>
#include "SingleFactor.h"

namespace DALM
{
class Logger;
class Vocabulary;
class LM;

typedef unsigned int VocabId;
}

namespace Moses
{
class Factor;

class LanguageModelDALM : public LanguageModelSingleFactor
{
protected:
	std::string m_inifile;
	DALM::Logger *m_logger;
	DALM::Vocabulary *m_vocab;
	DALM::LM *m_lm;

	DALM::VocabId wid_start, wid_end;

	typedef boost::bimap<const Factor *, DALM::VocabId> VocabMap;
	mutable VocabMap m_vocabMap;

	void CreateVocabMapping(const std::string &wordstxt);
	DALM::VocabId GetVocabId(const Factor *factor) const;

public:
	LanguageModelDALM(const std::string &line);
  ~LanguageModelDALM();
  void Load();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;
};


}
