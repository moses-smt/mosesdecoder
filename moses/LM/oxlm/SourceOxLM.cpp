#include "moses/LM/oxlm/SourceOxLM.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include "moses/TypeDef.h"
#include "moses/TranslationTask.h"

using namespace std;
using namespace oxlm;

namespace Moses
{

SourceOxLM::SourceOxLM(const string &line)
  : BilingualLM(line), posBackOff(false), posFactorType(1),
    persistentCache(false), cacheHits(0), totalHits(0)
{
  FactorCollection& factorFactory = FactorCollection::Instance(); // To add null word.
  const Factor* NULL_factor = factorFactory.AddFactor("<unk>");
  NULL_word.SetFactor(0, NULL_factor);
}

SourceOxLM::~SourceOxLM()
{
  if (persistentCache) {
    double cache_hit_ratio = 100.0 * cacheHits / totalHits;
    cerr << "Cache hit ratio: " << cache_hit_ratio << endl;
  }
}

float SourceOxLM::Score(
  vector<int>& source_words,
  vector<int>& target_words) const
{
  // OxLM expects the context in the following format:
  // [t_{n-1}, t_{n-2}, ..., t_{n-m}, s_{a_n-sm}, s_{a_n-sm+1}, ..., s_{a_n+sm}]
  // where n is the index for the current target word, m is the target order,
  // a_n is t_n's affiliation and sm is the source order.
  vector<int> context = target_words;
  int word = context.back();
  context.pop_back();
  reverse(context.begin(), context.end());
  context.insert(context.end(), source_words.begin(), source_words.end());

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
      score = model.getLogProb(word, context);
      cache->put(query, score);
    }
  } else {
    score = model.getLogProb(word, context);
  }

  // TODO(pauldb): Return OOV count too.
  return score;
}

int SourceOxLM::getNeuralLMId(const Word& word, bool is_source_word) const
{
  return is_source_word ? mapper->convertSource(word) : mapper->convert(word);
}

const Word& SourceOxLM::getNullWord() const
{
  return NULL_word;
}

void SourceOxLM::loadModel()
{
  model.load(m_filePath);

  boost::shared_ptr<ModelData> config = model.getConfig();
  source_ngrams = 2 * config->source_order - 1;
  target_ngrams = config->ngram_order - 1;

  boost::shared_ptr<Vocabulary> vocab = model.getVocab();
  mapper = boost::make_shared<OxLMParallelMapper>(
             vocab, posBackOff, posFactorType);
}

void SourceOxLM::SetParameter(const string& key, const string& value)
{
  if (key == "persistent-cache") {
    persistentCache = Scan<bool>(value);
  } else if (key == "pos-back-off") {
    posBackOff = Scan<bool>(value);
  } else if (key == "pos-factor-type") {
    posFactorType = Scan<FactorType>(value);
  } else {
    BilingualLM::SetParameter(key, value);
  }
}

void SourceOxLM::InitializeForInput(ttasksptr const& ttask)
{
  const InputType& source = *ttask->GetSource();
  BilingualLM::InitializeForInput(ttask);

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

void SourceOxLM::CleanUpAfterSentenceProcessing(const InputType& source)
{
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
