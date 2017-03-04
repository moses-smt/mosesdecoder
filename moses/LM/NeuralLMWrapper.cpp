
#include "moses/StaticData.h"
#include "moses/FactorCollection.h"
#include <boost/functional/hash.hpp>
#include "NeuralLMWrapper.h"
#include "neuralLM.h"

using namespace std;

namespace Moses
{
NeuralLMWrapper::NeuralLMWrapper(const std::string &line)
  :LanguageModelSingleFactor(line)
{
  ReadParameters();
}


NeuralLMWrapper::~NeuralLMWrapper()
{
  delete m_neuralLM_shared;
}


void NeuralLMWrapper::Load(AllOptions::ptr const& opts)
{

  // Set parameters required by ancestor classes
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartWord[m_factorType] = m_sentenceStart;
  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  m_neuralLM_shared = new nplm::neuralLM();
  m_neuralLM_shared->read(m_filePath);
  m_neuralLM_shared->premultiply();
  //TODO: config option?
  m_neuralLM_shared->set_cache(1000000);

  m_unk = m_neuralLM_shared->lookup_word("<unk>");

  UTIL_THROW_IF2(m_nGramOrder != m_neuralLM_shared->get_order(),
                 "Wrong order of neuralLM: LM has " << m_neuralLM_shared->get_order() << ", but Moses expects " << m_nGramOrder);

}


LMResult NeuralLMWrapper::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{

  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
    //TODO: config option?
    m_neuralLM->set_cache(1000000);
  }

  vector<int> words(contextFactor.size());
  const size_t n = contextFactor.size();
  for (size_t i=0; i<n; i++) {
    const Word* word = contextFactor[i];
    const Factor* factor = word->GetFactor(m_factorType);
    const std::string string = factor->GetString().as_string();
    int neuralLM_wordID = m_neuralLM->lookup_word(string);
    words[i] = neuralLM_wordID;
  }
  // Generate hashCode for only the last n-1 words, that represents the next LM
  // state
  size_t hashCode = 0;
  for (size_t i=1; i<n; ++i) {
    boost::hash_combine(hashCode, words[i]);
  }

  double value = m_neuralLM->lookup_ngram(words);

  // Create a new struct to hold the result
  LMResult ret;
  ret.score = FloorScore(value);
  ret.unknown = (words.back() == m_unk);

  (*finalState) = (State*) hashCode;

  return ret;
}

}


