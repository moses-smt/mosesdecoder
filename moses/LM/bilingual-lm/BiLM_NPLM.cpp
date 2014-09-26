#include "BiLM_NPLM.h"
#include "neuralLM.h"

namespace Moses {

  BilingualLM_NPLM::BilingualLM_NPLM(const std::string &line) 
    :BilingualLM(line)
    ,premultiply(true)
    ,neuralLM_cache(1000000) {};

  float BilingualLM_NPLM::Score(std::vector<int>& source_words, std::vector<int>& target_words) const {
    source_words.reserve(source_ngrams+target_ngrams+1);
    source_words.insert( source_words.end(), target_words.begin(), target_words.end() );
    return m_neuralLM->lookup_ngram(source_words);
  }

  int BilingualLM_NPLM::LookUpNeuralLMWord(const std::string& str) const {
    return m_neuralLM->lookup_word(str);
  }
    
  void BilingualLM_NPLM::initSharedPointer() const {
    if (!m_neuralLM.get()) {
      m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
    }
  }

  void BilingualLM_NPLM::SetParameter(const std::string& key, const std::string& value) {
    if (key == "ngrams") {
      m_nGramOrder = Scan<int>(value);
    } else if (key == "target_ngrams") {
      target_ngrams = Scan<int>(value);
    } else if (key == "source_ngrams") {
      source_ngrams = Scan<int>(value);
    } else if (key == "factored") {
      factored = Scan<bool>(value);
    } else if (key == "pos_factor") {
      pos_factortype = Scan<FactorType>(value);
    } else if (key == "cache_size") {
      neuralLM_cache = atoi(value.c_str());
    } else if (key == "premultiply") {
      premultiply = Scan<bool>(value);
    } else {
      BilingualLM::SetParameter(key, value);
    }
  }

  void BilingualLM_NPLM::loadModel() {
    m_neuralLM_shared = new nplm::neuralLM(m_filePath, premultiply); //Default premultiply= true

    m_neuralLM_shared->set_cache(neuralLM_cache); //Default 1000000
    UTIL_THROW_IF2(m_nGramOrder != m_neuralLM_shared->get_order(),
                   "Wrong order of neuralLM: LM has " << m_neuralLM_shared->get_order() << ", but Moses expects " << m_nGramOrder);
  }

};
