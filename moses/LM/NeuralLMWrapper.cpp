
#include "moses/StaticData.h"
#include "moses/FactorCollection.h"
#include "NeuralLMWrapper.h"
#include "neuralLM.h"
#include <model.h>

using namespace std;

namespace Moses
{
NeuralLMWrapper::NeuralLMWrapper(const std::string &line)
:LanguageModelSingleFactor(line)
{
  // This space intentionally left blank
}


NeuralLMWrapper::~NeuralLMWrapper()
{
  delete m_neuralLM;
}


void NeuralLMWrapper::Load()
{

  TRACE_ERR("Loading NeuralLM " << m_filePath << endl);

  // Set parameters required by ancestor classes
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartWord[m_factorType] = m_sentenceStart;
  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  m_neuralLM = new nplm::neuralLM();
  m_neuralLM->read(m_filePath);
  m_neuralLM->set_log_base(10);

  //TODO: Implement this
}


LMResult NeuralLMWrapper::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{

  unsigned int hashCode = 0;
  vector<int> words(contextFactor.size());
//  TRACE_ERR("NeuralLM words:");
  for (size_t i=0, n=contextFactor.size(); i<n; i+=1) {
    const Word* word = contextFactor[i];
    const Factor* factor = word->GetFactor(m_factorType);
    const std::string string= factor->GetString().as_string();
    int neuralLM_wordID = m_neuralLM->lookup_word(string);
    words[i] = neuralLM_wordID;
    hashCode += neuralLM_wordID;
//    TRACE_ERR(" " << string << "(" << neuralLM_wordID << ")" );
  }

  double value = m_neuralLM->lookup_ngram(words);
//  TRACE_ERR("\t=\t" << value);
//  TRACE_ERR(endl);

  // Create a new struct to hold the result
  LMResult ret;
  ret.score = value;
  ret.unknown = false;


  // State* finalState is a void pointer
  //
  // Construct a hash value from the vector of words (contextFactor)
  //
  // The hash value must be the same size as sizeof(void*)
  //
  // TODO Set finalState to the above hash value

  // use last word as state info
//  const Factor *factor;
//  size_t hash_value(const Factor &f);
//  if (contextFactor.size()) {
//    factor = contextFactor.back()->GetFactor(m_factorType);
//  } else {
//    factor = NULL;
//  }
//
//  (*finalState) = (State*) factor;

  (*finalState) = (State*) hashCode;

  return ret;
}

}


