#include "LBLLM.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>

#include "moses/FactorCollection.h"
#include "moses/InputType.h"

using namespace std;
using namespace oxlm;

namespace Moses
{

template<class Model>
LBLLM<Model>::LBLLM(const string &line) : LanguageModelSingleFactor(line) {
  ReadParameters();

  FactorCollection &factorCollection = FactorCollection::Instance();

  // needed by parent language model classes. Why didn't they set these themselves?
  m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
  m_sentenceStartWord[m_factorType] = m_sentenceStart;

  m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
  m_sentenceEndWord[m_factorType] = m_sentenceEnd;

  cacheHits = totalHits = 0;
}


template<class Model>
LBLLM<Model>::~LBLLM() {
  if (persistentCache) {
    double cache_hit_ratio = 100.0 * cacheHits / totalHits;
    cerr << "Cache hit ratio: " << cache_hit_ratio << endl;
  }
}


template<class Model>
void LBLLM<Model>::SetParameter(const string& key, const string& value) {
  if (key == "persistent-cache") {
    persistentCache = Scan<bool>(value);
  } else {
    LanguageModelSingleFactor::SetParameter(key, value);
  }
}

template<class Model>
void LBLLM<Model>::Load() {
  model.load(m_filePath);

  Dict dict = model.getDict();
  mapper = boost::make_shared<OXLMMapper>(dict);

  kSTART = dict.Convert("<s>");
  kSTOP = dict.Convert("</s>");
  kUNKNOWN = dict.Convert("<unk>");

  size_t ngram_order = model.getConfig()->ngram_order;
  UTIL_THROW_IF2(
      m_nGramOrder != ngram_order,
      "Wrong order for LBLLM: LM has " << ngram_order << ", but Moses expects " << m_nGramOrder);
}

template<class Model>
LMResult LBLLM<Model>::GetValue(
    const vector<const Word*> &contextFactor, State* finalState) const {
  if (!cache.get()) {
    cache.reset(new QueryCache());
  }

  vector<int> context;
  int word;
  mapper->convert(contextFactor, context, word);

  size_t context_width = m_nGramOrder - 1;

  if (!context.empty() && context.back() == kSTART) {
    context.resize(context_width, kSTART);
  } else {
    context.resize(context_width, kUNKNOWN);
  }


  double score;
  if (persistentCache) {
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

  LMResult ret;
  ret.score = score;
  ret.unknown = (word == kUNKNOWN);

  // calc state from hash of last n-1 words
  size_t seed = 0;
  boost::hash_combine(seed, word);
  for (size_t i = 0; i < context.size() && i < context_width - 1; ++i) {
    int id = context[i];
    boost::hash_combine(seed, id);
  }

  (*finalState) = (State*) seed;
  return ret;
}

template<class Model>
void LBLLM<Model>::InitializeForInput(const InputType& source) {
  LanguageModelSingleFactor::InitializeForInput(source);

  if (persistentCache) {
    if (!cache.get()) {
      cache.reset(new QueryCache());
    }

    int sentence_id = source.GetTranslationId();
    string cacheFile = m_filePath + "." + to_string(sentence_id) + ".cache.bin";
    if (boost::filesystem::exists(cacheFile)) {
      ifstream f(cacheFile);
      boost::archive::binary_iarchive iar(f);
      cerr << "Loading n-gram probability cache from " << cacheFile << endl;
      iar >> *cache;
      cerr << "Done loading " << cache->size()
           << " n-gram probabilities..." << endl;
    } else {
      cerr << "Cache file not found" << endl;
    }
  }
}

template<class Model>
void LBLLM<Model>::CleanUpAfterSentenceProcessing(const InputType& source) {
  model.clearCache();

  if (persistentCache) {
    int sentence_id = source.GetTranslationId();
    string cacheFile = m_filePath + "." + to_string(sentence_id) + ".cache.bin";
    ofstream f(cacheFile);
    boost::archive::binary_oarchive oar(f);
    cerr << "Saving persistent cache to " << cacheFile << endl;
    oar << *cache;
    cerr << "Done saving " << cache->size()
         << " n-gram probabilities..." << endl;

    cache->clear();
  }

  LanguageModelSingleFactor::CleanUpAfterSentenceProcessing(source);
}

template class LBLLM<LM>;
template class LBLLM<FactoredLM>;
template class LBLLM<FactoredMaxentLM>;

}



