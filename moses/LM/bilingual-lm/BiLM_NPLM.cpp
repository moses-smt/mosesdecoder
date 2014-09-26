#include "BiLM_NPLM.h"
#include "neuralLM.h"

namespace Moses {

BilingualLM_NPLM::BilingualLM_NPLM(const std::string &line)
      : BilingualLM(line),
        premultiply(true),
        factored(false),
        neuralLM_cache(1000000) {}

float BilingualLM_NPLM::Score(std::vector<int>& source_words, std::vector<int>& target_words) const {
  source_words.reserve(source_ngrams+target_ngrams+1);
  source_words.insert( source_words.end(), target_words.begin(), target_words.end() );
  return m_neuralLM->lookup_ngram(source_words);
}

int BilingualLM_NPLM::LookUpNeuralLMWord(const std::string& str) const {
  return m_neuralLM->lookup_word(str);
}

//Cache for NeuralLMids
int BilingualLM_NPLM::getNeuralLMId(
    const Word& word, bool is_source_word) const{
  initSharedPointer();

  const Factor* factor = word.GetFactor(word_factortype);

  std::map<const Factor *, int>::iterator it;

  boost::upgrade_lock< boost::shared_mutex > read_lock(neuralLMids_lock);
  it = neuralLMids.find(factor);

  if (it != neuralLMids.end()) {
    if (!factored){
      return it->second; //Lock is released here automatically
    } else {
      //See if word is unknown
      if (it->second == unknown_word_id){
        const Factor* pos_factor = word.GetFactor(pos_factortype); //Get POS tag
        //Look up the POS tag in the cache
        it = neuralLMids.find(pos_factor);
        if (it != neuralLMids.end()){
          return it->second; //We have our pos tag in the cache.
        } else {
          //We have to lookup the pos_tag
          const std::string posstring = pos_factor->GetString().as_string();
          int neuralLM_wordID = LookUpNeuralLMWord(posstring);

          boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(read_lock);
          neuralLMids.insert(std::pair<const Factor *, int>(pos_factor, neuralLM_wordID));

          return neuralLM_wordID; //We return the ID of the pos TAG
        }
      } else {
        return it->second; //We return the neuralLMid of the word
      }
    }
  } else {
    //We have to lookup the word
    const std::string string = factor->GetString().as_string();
    int neuralLM_wordID = LookUpNeuralLMWord(string);

    boost::upgrade_to_unique_lock< boost::shared_mutex > uniqueLock(read_lock);
    neuralLMids.insert(std::pair<const Factor *, int>(factor, neuralLM_wordID));

    if (!factored) {
      return neuralLM_wordID; //Lock is released here
    } else {
      if (neuralLM_wordID == unknown_word_id){
        const Factor* pos_factor = word.GetFactor(pos_factortype);
        const std::string factorstring = pos_factor->GetString().as_string();
        neuralLM_wordID = LookUpNeuralLMWord(string);
        neuralLMids.insert(std::pair<const Factor *, int>(pos_factor, neuralLM_wordID));
      }
      return neuralLM_wordID; //If a POS tag is needed, neuralLM_wordID is going to be updated.
    }
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
}

} // namespace Moses
