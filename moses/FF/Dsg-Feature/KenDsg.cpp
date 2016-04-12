#include "KenDsg.h"

namespace Moses
{

DsgLM* ConstructDsgLM(const char *file)
{
  lm::ngram::ModelType model_type;
  lm::ngram::Config config;
  if (lm::ngram::RecognizeBinary(file, model_type)) {
    switch(model_type) {
    case lm::ngram::PROBING:
      return new KenDsg<lm::ngram::ProbingModel>(file, config);
    case lm::ngram::REST_PROBING:
      return new KenDsg<lm::ngram::RestProbingModel>(file, config);
    case lm::ngram::TRIE:
      return new KenDsg<lm::ngram::TrieModel>(file, config);
    case lm::ngram::QUANT_TRIE:
      return new KenDsg<lm::ngram::QuantTrieModel>(file, config);
    case lm::ngram::ARRAY_TRIE:
      return new KenDsg<lm::ngram::ArrayTrieModel>(file, config);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new KenDsg<lm::ngram::QuantArrayTrieModel>(file, config);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type);
    }
  } else {
    return new KenDsg<lm::ngram::ProbingModel>(file, config);
  }
}

} // namespace


