#pragma once

#include "SingleFactor.h"

namespace nplm {
  class neuralLM;
}

namespace Moses
{

/** Implementation of single factor LM using IRST's code.
 */
class NeuralLMWrapper : public LanguageModelSingleFactor
{
protected:
  nplm::neuralLM *m_neuralLM;

public:
  NeuralLMWrapper(const std::string &line);
  //  NeuralLM(const std::string &line);
  ~NeuralLMWrapper();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;

  virtual void Load();

};


} // namespace





