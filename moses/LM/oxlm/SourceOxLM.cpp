#include "moses/LM/oxlm/SourceOxLM.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace oxlm;

namespace Moses {

SourceOxLM::SourceOxLM(const string &line)
    : BilingualLM(line), cacheHits(0), totalHits(0) {}

SourceOxLM::~SourceOxLM() {
  if (persistentCache) {
    double cache_hit_ratio = 100.0 * cacheHits / totalHits;
    cerr << "Cache hit ratio: " << cache_hit_ratio << endl;
  }
}

float SourceOxLM::Score(
    vector<int>& source_words,
    vector<int>& target_words) const {
  // OxLM expects the context in the following format:
  // [t_{n-1}, t_{n-2}, ..., t_{n-m}, s_{a_n-sm}, s_{a_n-sm+1}, ..., s_{a_n+sm}]
  // where n is the index for the current target word, m is the target order,
  // a_n is t_n's affiliation and sm is the source order.
  vector<int> context = target_words;
  int word = context.back();
  context.pop_back();
  reverse(context.begin(), context.end());
  context.insert(context.end(), source_words.begin(), source_words.end());

  /*
  boost::shared_ptr<oxlm::Vocabulary> vocab = model.getVocab();
  boost::shared_ptr<oxlm::ParallelVocabulary> parallel_vocab =
      dynamic_pointer_cast<oxlm::ParallelVocabulary>(vocab);
  assert(parallel_vocab != nullptr);

  cerr << parallel_vocab->convert(word) << " ||| ";
  for (int i = 0; i < target_ngrams; ++i) {
    cerr << parallel_vocab->convert(context[i]) << " ";
  }
  cerr << "||| ";
  for (int i = 0; i < source_ngrams; ++i) {
    cerr << parallel_vocab->convertSource(context[target_ngrams + i]) << " ";
  }
  */

  float score;
  if (persistentCache) {
    if (!cache.get()) {
      cache.reset(new QueryCache());
    }

    ++totalHits;
    NGram query(word, context);
    pair<double, bool> ret = cache->get(query);
    if (ret.second) {
      score = ret.first;
      ++cacheHits;
    } else {
      score = model.predict(word, context);
      cache->put(query, score);
    }
  } else {
    score = model.predict(word, context);
  }

  // cerr << "||| " << exp(score) << endl;

  // TODO(pauldb): Return OOV count too.
  return score;
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

void SourceOxLM::InitializeForInput(const InputType& source) {
  BilingualLM::InitializeForInput(source);

  if (persistentCache) {
    if (!cache.get()) {
      cache.reset(new QueryCache());
    }

    int sentence_id = source.GetTranslationId();
    string cacheFile = m_filePath + "." + to_string(sentence_id) + ".cache.bin";
    if (boost::filesystem::exists(cacheFile)) {
      ifstream fin(cacheFile);
      boost::archive::binary_iarchive iar(fin);
      cerr << "Loading n-gram probability cache from " << cacheFile << endl;
      iar >> *cache;
      cerr << "Done loading " << cache->size()
           << " n-gram probabilities..." << endl;
    } else {
      cerr << "Cache file not found!" << endl;
    }
  }
}

void SourceOxLM::CleanUpAfterSentenceProcessing(const InputType& source) {
  // Thread safe: the model cache is thread specific.
  model.clearCache();

  if (persistentCache) {
    int sentence_id = source.GetTranslationId();
    string cacheFile = m_filePath + "." + to_string(sentence_id) + ".cache.bin";
    ofstream fout(cacheFile);
    boost::archive::binary_oarchive oar(fout);
    cerr << "Saving persistent cache to " << cacheFile << endl;
    oar << *cache;
    cerr << "Done saving " << cache->size()
         << " n-gram probabilities..." << endl;

    cache->clear();
  }
}

} // namespace Moses
