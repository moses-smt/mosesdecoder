
#include "ExampleLM.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
ExampleLM::ExampleLM(const std::string &line)
  :LanguageModelSingleFactor(line)
{
  ReadParameters();

  UTIL_THROW_IF2(m_nGramOrder == NOT_FOUND, "Must set order");
  UTIL_THROW_IF2(m_nGramOrder <= 1, "Ngram order must be more than 1");

  FactorCollection &factorCollection = FactorCollection::Instance();

  // needed by parent language model classes. Why didn't they set these themselves?
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;
}

ExampleLM::~ExampleLM()
{
}

LMResult ExampleLM::GetValue(const vector<const Word*> &contextFactor, State* finalState) const
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

}



