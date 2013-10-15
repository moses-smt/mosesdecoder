
#include "moses/StaticData.h"
#include "NeuralLM.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
NeuralLM::NeuralLM()
  //NeuralLM::NeuralLM(const std::string &line)
    //  :LanguageModelSingleFactor("NeuralLM", line)
{

  //ReadParameters();

}


NeuralLM::~NeuralLM()
{
}


bool NeuralLM::Load(const std::string &filePath, FactorType factorType, size_t nGramOrder) 
{

  TRACE_ERR("Loading NeuralLM " << filePath << endl);
  m_factorType = factorType;
  m_nGramOrder = nGramOrder;
  m_filePath = filePath;
  
  if (factorType == NOT_FOUND) {
    m_factorType = 0;
  }
  
  FactorCollection &factorCollection = FactorCollection::Instance();
  
  // needed by parent language model classes. Why didn't they set these themselves?
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartArray[m_factorType] = m_sentenceStart;
  
  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndArray[m_factorType] = m_sentenceEnd;  
  
  return true;
  //TODO: Implement this
}


LMResult NeuralLM::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
{
  // Create a new struct to hold the result
  LMResult ret;
  ret.score = contextFactor.size(); //TODO Change from dummy value to actual LM score
  //TRACE_ERR("ret.score = " << ret.score << endl);
  ret.unknown = false;

  // State* finalState is a void pointer
  //
  // Construct a hash value from the vector of words (contextFactor)
  // 
  // The hash value must be the same size as sizeof(void*)
  // 
  // TODO Set finalState to the above hash value
  
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

}



