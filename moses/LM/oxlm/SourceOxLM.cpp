#include "moses/LM/oxlm/SourceOxLM.h"

using namespace std;
using namespace oxlm;

namespace Moses {

SourceOxLM::SourceOxLM(const string &line) : BilingualLM(line) {
}

SourceOxLM::~SourceOxLM() {}

float SourceOxLM::Score(
    vector<int>& source_words,
    vector<int>& target_words) const {
  vector<int> context = target_words;
  int word = context.front();
  context.erase(context.begin());
  context.insert(context.end(), source_words.begin(), source_words.end());

  return model.predict(word, context);
}

int SourceOxLM::getNeuralLMId(const Word& word, bool is_source_word) const {
  const Moses::Factor* factor = word.GetFactor(0);
  return is_source_word ?
      mapper->convertSource(factor) : mapper->convert(factor);
}

void SourceOxLM::loadModel() {
  model.load(m_filePath);

  boost::shared_ptr<ModelData> config = model.getConfig();
  source_ngrams = 2 * config->source_order - 1;
  target_ngrams = config->ngram_order - 1;

  boost::shared_ptr<Vocabulary> vocab = model.getVocab();
  mapper = boost::make_shared<OxLMParallelMapper>(vocab);
}

void SourceOxLM::SetParameter(const string& key, const string& value) {
  if (key == "persistent-cache") {
    persistentCache = Scan<bool>(value);
  } else {
    BilingualLM::SetParameter(key, value);
  }
}

} // namespace Moses



