#include "BiLM_NPLM.h"
#include "neuralLM.h"
#include "vocabulary.h"

namespace Moses {

BilingualLM_NPLM::BilingualLM_NPLM(const std::string &line)
      : BilingualLM(line),
        premultiply(true),
        factored(false),
        neuralLM_cache(1000000) {
          
          if (!NULL_overwrite) {
            NULL_string = "<null>"; //Default null value for nplm
          }
          FactorCollection& factorFactory = FactorCollection::Instance(); // To add null word.
          const Factor* NULL_factor = factorFactory.AddFactor(NULL_string);
          NULL_word.SetFactor(0, NULL_factor);
        }

float BilingualLM_NPLM::Score(std::vector<int>& source_words, std::vector<int>& target_words) const {
  source_words.reserve(source_ngrams+target_ngrams+1);
  source_words.insert( source_words.end(), target_words.begin(), target_words.end() );
  return FloorScore(m_neuralLM->lookup_ngram(source_words));
}

const Word& BilingualLM_NPLM::getNullWord() const {
  return NULL_word;
}

int BilingualLM_NPLM::getNeuralLMId(const Word& word, bool is_source_word) const {
  initSharedPointer();

  boost::unordered_map<const Factor*, int>::iterator it;
  const Factor* factor = word.GetFactor(word_factortype);

  it = neuralLMids.find(factor);
  //If we know the word return immediately
  if (it != neuralLMids.end()){
    return it->second;
  }
  //If we don't know the word and we aren't factored, return the word.
  if (!factored) {
      return unknown_word_id;
  } 
  //Else try to get a pos_factor
  const Factor* pos_factor = word.GetFactor(pos_factortype);
  it = neuralLMids.find(pos_factor);
  if (it != neuralLMids.end()){
    return it->second;
  } else {
    return unknown_word_id;
  }
}

void BilingualLM_NPLM::initSharedPointer() const {
  if (!m_neuralLM.get()) {
    m_neuralLM.reset(new nplm::neuralLM(*m_neuralLM_shared));
  }
}

void BilingualLM_NPLM::SetParameter(const std::string& key, const std::string& value) {
  if (key == "target_ngrams") {
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
  } else if (key == "null_word") {
    NULL_string = value;
    NULL_overwrite = true;
  } else {
    BilingualLM::SetParameter(key, value);
  }
}

void BilingualLM_NPLM::loadModel() {
  m_neuralLM_shared = new nplm::neuralLM(m_filePath, premultiply); //Default premultiply= true

  int ngram_order = target_ngrams + source_ngrams + 1;
  UTIL_THROW_IF2(
      ngram_order != m_neuralLM_shared->get_order(),
      "Wrong order of neuralLM: LM has " << m_neuralLM_shared->get_order() <<
      ", but Moses expects " << ngram_order);

  m_neuralLM_shared->set_cache(neuralLM_cache); //Default 1000000
  unknown_word_id = m_neuralLM_shared->lookup_word("<unk>");

  //Setup factor -> NeuralLMId cache
  FactorCollection& factorFactory = FactorCollection::Instance(); //To do the conversion from string to vocabID

  const nplm::vocabulary& vocab = m_neuralLM_shared->get_vocabulary();
  const boost::unordered_map<std::string, int>& neuraLMvocabmap = vocab.get_idmap();

  boost::unordered_map<std::string, int>::const_iterator it;

  for (it = neuraLMvocabmap.cbegin(); it != neuraLMvocabmap.cend(); it++) {
    std::string raw_word = it->first;
    int neuralLMid = it->second;
    const Factor * factor = factorFactory.AddFactor(raw_word);

    neuralLMids.insert(std::make_pair(factor, neuralLMid));
  }

}

} // namespace Moses
