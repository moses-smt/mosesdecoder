// $Id$
#pragma once

#include <vector>
#include "SingleFactor.h"

namespace nplm {
  class neuralLM; 
}

namespace Moses
{

class NeuralLM : public LanguageModelPointerState
{
protected:
  nplm::neuralLM *m_neuralLM;
  
public:
  NeuralLM();
  //  NeuralLM(const std::string &line);
  ~NeuralLM();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;

  virtual bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);
};


}
