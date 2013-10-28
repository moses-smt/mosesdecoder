#pragma once

#include "SingleFactor.h"

namespace nplm {
  class neuralLM;
}

namespace Moses
{

/** Implementation of single factor LM using IRST's code.
 */
class NeuralLM : public LanguageModelSingleFactor
{
protected:
  nplm::neuralLM *m_neuralLM;

public:
  NeuralLM(const std::string &line);
  //  NeuralLM(const std::string &line);
  ~NeuralLM();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;

  virtual bool Load(const std::string &filePath, FactorType factorType, size_t nGramOrder);

};


} // namespace





