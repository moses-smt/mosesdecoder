#include "KenESM.h"

namespace Moses
{

ESMLM* ConstructESMLM(const std::string &file)
{
    lm::ngram::ModelType model_type;
    if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {

      switch(model_type) {
      case lm::ngram::PROBING:
        return new KenESM<lm::ngram::ProbingModel>(file);
      case lm::ngram::REST_PROBING:
        return new KenESM<lm::ngram::RestProbingModel>(file);
      case lm::ngram::TRIE:
        return new KenESM<lm::ngram::TrieModel>(file);
      case lm::ngram::QUANT_TRIE:
        return new KenESM<lm::ngram::QuantTrieModel>(file);
      case lm::ngram::ARRAY_TRIE:
        return new KenESM<lm::ngram::ArrayTrieModel>(file);
      case lm::ngram::QUANT_ARRAY_TRIE:
        return new KenESM<lm::ngram::QuantArrayTrieModel>(file);
      default:
    	UTIL_THROW2("Unrecognized kenlm model type " << model_type);
      }
    } else {
      return new KenESM<lm::ngram::ProbingModel>(file);
    }
}

} // namespace
