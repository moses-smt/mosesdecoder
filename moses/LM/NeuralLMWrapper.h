#pragma once

#include "SingleFactor.h"

#include <boost/thread/tss.hpp>

namespace nplm
{
class neuralLM;
}

namespace Moses
{

class NeuralLMWrapper : public LanguageModelSingleFactor
{
protected:
  // big data (vocab, weights, cache) shared among threads
  nplm::neuralLM *m_neuralLM_shared;
  // thread-specific nplm for thread-safety
  mutable boost::thread_specific_ptr<nplm::neuralLM> m_neuralLM;
  int m_unk;

public:
  NeuralLMWrapper(const std::string &line);
  ~NeuralLMWrapper();

  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const;

  virtual void Load();

};


} // namespace





