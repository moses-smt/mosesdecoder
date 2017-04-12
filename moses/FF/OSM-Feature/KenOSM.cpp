#include "KenOSM.h"

namespace Moses
{

OSMLM* ConstructOSMLM(const char *file, util::LoadMethod load_method)
{
  lm::ngram::ModelType model_type;
  lm::ngram::Config config;
  config.load_method = load_method;
  if (lm::ngram::RecognizeBinary(file, model_type)) {
    switch(model_type) {
    case lm::ngram::PROBING:
      return new KenOSM<lm::ngram::ProbingModel>(file, config);
    case lm::ngram::REST_PROBING:
      return new KenOSM<lm::ngram::RestProbingModel>(file, config);
    case lm::ngram::TRIE:
      return new KenOSM<lm::ngram::TrieModel>(file, config);
    case lm::ngram::QUANT_TRIE:
      return new KenOSM<lm::ngram::QuantTrieModel>(file, config);
    case lm::ngram::ARRAY_TRIE:
      return new KenOSM<lm::ngram::ArrayTrieModel>(file, config);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new KenOSM<lm::ngram::QuantArrayTrieModel>(file, config);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type);
    }
  } else {
    return new KenOSM<lm::ngram::ProbingModel>(file, config);
  }
}

} // namespace
