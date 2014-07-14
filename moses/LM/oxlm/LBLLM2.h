// $Id$
#pragma once

#include <vector>
#include "moses/LM/SingleFactor.h"
#include "moses/FactorCollection.h"

namespace Moses
{

template<class Model>
class LBLLM2 : public LanguageModelSingleFactor
{
protected:

public:
	LBLLM2(const std::string &line)
	:LanguageModelSingleFactor(line)
	{
		ReadParameters();

		FactorCollection &factorCollection = FactorCollection::Instance();

		// needed by parent language model classes. Why didn't they set these themselves?
		m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
		m_sentenceStartWord[m_factorType] = m_sentenceStart;

		m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
		m_sentenceEndWord[m_factorType] = m_sentenceEnd;
	}

  ~LBLLM2()
  {}

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const
  {
    LMResult ret;
    ret.score = contextFactor.size();
    ret.unknown = false;

    // use last word as state info
    const Factor *factor;
    size_t hash_value(const Factor &f);
    if (contextFactor.size()) {
      factor = contextFactor.back()->GetFactor(m_factorType);
    } else {
      factor = NULL;
    }

    (*finalState) = (State*) factor;

    return ret;
  }

};


}
