//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

// This file should be compiled only when the LM_RAND flag is enabled.
//
// The following ifdef prevents XCode and other non-bjam build systems
// from attempting to compile this file when LM_RAND is disabled.
//
#ifdef LM_RAND

#include "LM/Base.h"
#include "LM/LDHT.h"
#include "moses/FFState.h"
#include "moses/TypeDef.h"
#include "moses/Hypothesis.h"
#include "moses/StaticData.h"
#include "util/exception.hh"

#include <LDHT/Client.h>
#include <LDHT/ClientLocal.h>
#include <LDHT/NewNgram.h>
#include <LDHT/FactoryCollection.h>

#include <boost/thread/tss.hpp>

namespace Moses
{

struct LDHTLMState : public FFState {
  LDHT::NewNgram gram_fingerprints;
  bool finalised;
  std::vector<int> request_tags;

  LDHTLMState(): finalised(false) {
  }

  void setFinalised() {
    this->finalised = true;
  }

  void appendRequestTag(int tag) {
    this->request_tags.push_back(tag);
  }

  void clearRequestTags() {
    this->request_tags.clear();
  }

  std::vector<int>::iterator requestTagsBegin() {
    return this->request_tags.begin();
  }

  std::vector<int>::iterator requestTagsEnd() {
    return this->request_tags.end();
  }

  int Compare(const FFState& uncast_other) const {
    const LDHTLMState &other = static_cast<const LDHTLMState&>(uncast_other);
    //if (!this->finalised)
    //    return -1;

    return gram_fingerprints.compareMoses(other.gram_fingerprints);
  }

  void copyFrom(const LDHTLMState& other) {
    gram_fingerprints.copyFrom(other.gram_fingerprints);
    finalised = false;
  }
};

class LanguageModelLDHT : public LanguageModel
{
public:
  LanguageModelLDHT();
  LanguageModelLDHT(const std::string& path,
                    ScoreIndexManager& manager,
                    FactorType factorType);
  LanguageModelLDHT(ScoreIndexManager& manager,
                    LanguageModelLDHT& copyFrom);

  LDHT::Client* getClientUnsafe() const;
  LDHT::Client* getClientSafe();
  LDHT::Client* initTSSClient();
  virtual ~LanguageModelLDHT();
  virtual void InitializeForInput(InputType const& source);
  virtual void CleanUpAfterSentenceProcessing(const InputType &source);
  virtual const FFState* EmptyHypothesisState(const InputType& input) const;
  virtual void CalcScore(const Phrase& phrase,
                         float& fullScore,
                         float& ngramScore,
                         std::size_t& oovCount) const;
  virtual void CalcScoreFromCache(const Phrase& phrase,
                                  float& fullScore,
                                  float& ngramScore,
                                  std::size_t& oovCount) const;
  FFState* Evaluate(const Hypothesis& hypo,
                    const FFState* input_state,
                    ScoreComponentCollection* score_output) const;
  FFState* EvaluateWhenApplied(const ChartHypothesis& hypo,
                               int featureID,
                               ScoreComponentCollection* accumulator) const;

  virtual void IssueRequestsFor(Hypothesis& hypo,
                                const FFState* input_state);
  float calcScoreFromState(LDHTLMState* hypo) const;
  void sync();
  void SetFFStateIdx(int state_idx);

protected:
  boost::thread_specific_ptr<LDHT::Client> m_client;
  std::string m_configPath;
  FactorType m_factorType;
  int m_state_idx;
  int m_calc_score_count;
  uint64_t m_start_tick;

};

LanguageModel* ConstructLDHTLM(const std::string& path,
                               ScoreIndexManager& manager,
                               FactorType factorType)
{
  return new LanguageModelLDHT(path, manager, factorType);
}

LanguageModelLDHT::LanguageModelLDHT() : LanguageModel(), m_client(NULL)
{
  m_enableOOVFeature = false;
}

LanguageModelLDHT::LanguageModelLDHT(ScoreIndexManager& manager,
                                     LanguageModelLDHT& copyFrom)
{
  m_calc_score_count = 0;
  //m_client = copyFrom.m_client;
  m_factorType = copyFrom.m_factorType;
  m_configPath = copyFrom.m_configPath;
  Init(manager);
}

LanguageModelLDHT::LanguageModelLDHT(const std::string& path,
                                     ScoreIndexManager& manager,
                                     FactorType factorType)
  : m_factorType(factorType)
{
  m_configPath = path;
  Init(manager);
}

LanguageModelLDHT::~LanguageModelLDHT()
{
  // TODO(wilson): should cleanup for each individual thread.
  //delete getClientSafe();
}

// Check that there is a TSS Client instance, and instantiate one if
// there isn't.
LDHT::Client* LanguageModelLDHT::getClientSafe()
{
  if (m_client.get() == NULL)
    m_client.reset(initTSSClient());
  return m_client.get();
}

// Do not check that there is a TSS Client instance.
LDHT::Client* LanguageModelLDHT::getClientUnsafe() const
{
  return m_client.get();
}

LDHT::Client* LanguageModelLDHT::initTSSClient()
{
  std::ifstream config_file(m_configPath.c_str());
  std::string ldht_config_path;
  getline(config_file, ldht_config_path);
  std::string ldhtlm_config_path;
  getline(config_file, ldhtlm_config_path);

  LDHT::FactoryCollection* factory_collection =
    LDHT::FactoryCollection::createDefaultFactoryCollection();

  LDHT::Client* client;
  //client = new LDHT::ClientLocal();
  client = new LDHT::Client();
  client->fromXmlFiles(*factory_collection,
                       ldht_config_path,
                       ldhtlm_config_path);
  return client;
}

void LanguageModelLDHT::InitializeForInput(InputType const& source)
{
  getClientSafe()->clearCache();
  m_start_tick = LDHT::Util::rdtsc();
}

void LanguageModelLDHT::CleanUpAfterSentenceProcessing(const InputType &source)
{
  LDHT::Client* client = getClientSafe();

  std::cerr << "LDHT sentence stats:" << std::endl;
  std::cerr << "  ngrams submitted: " << client->getNumNgramsSubmitted() << std::endl
            << "  ngrams requested: " << client->getNumNgramsRequested() << std::endl
            << "  ngrams not found: " << client->getKeyNotFoundCount() << std::endl
            << "        cache hits: " << client->getCacheHitCount() << std::endl
            << "        inferences: " << client->getInferenceCount() << std::endl
            << "      pcnt latency: " << (float)client->getLatencyTicks() / (float)(LDHT::Util::rdtsc() - m_start_tick) * 100.0 << std::endl;
  m_start_tick = 0;
  client->resetLatencyTicks();
  client->resetNumNgramsSubmitted();
  client->resetNumNgramsRequested();
  client->resetInferenceCount();
  client->resetCacheHitCount();
  client->resetKeyNotFoundCount();
}

const FFState* LanguageModelLDHT::EmptyHypothesisState(
  const InputType& input) const
{
  return NULL;
}

void LanguageModelLDHT::CalcScore(const Phrase& phrase,
                                  float& fullScore,
                                  float& ngramScore,
                                  std::size_t& oovCount) const
{
  const_cast<LanguageModelLDHT*>(this)->m_calc_score_count++;
  if (m_calc_score_count > 10000) {
    const_cast<LanguageModelLDHT*>(this)->m_calc_score_count = 0;
    const_cast<LanguageModelLDHT*>(this)->sync();
  }

  // TODO(wilson): handle nonterminal words.
  LDHT::Client* client = getClientUnsafe();
  // Score the first order - 1 words of the phrase.
  int order = LDHT::NewNgram::k_max_order;
  int prefix_start = 0;
  int prefix_end = std::min(phrase.GetSize(), static_cast<size_t>(order - 1));
  LDHT::NewNgram ngram;
  for (int word_idx = prefix_start; word_idx < prefix_end; ++word_idx) {
    ngram.appendGram(phrase.GetWord(word_idx)
                     .GetFactor(m_factorType)->GetString().c_str());
    client->requestNgram(ngram);
  }
  // Now score all subsequent ngrams to end of phrase.
  int internal_start = prefix_end;
  int internal_end = phrase.GetSize();
  for (int word_idx = internal_start; word_idx < internal_end; ++word_idx) {
    ngram.appendGram(phrase.GetWord(word_idx)
                     .GetFactor(m_factorType)->GetString().c_str());
    client->requestNgram(ngram);
  }

  fullScore = 0;
  ngramScore = 0;
  oovCount = 0;
}

void LanguageModelLDHT::CalcScoreFromCache(const Phrase& phrase,
    float& fullScore,
    float& ngramScore,
    std::size_t& oovCount) const
{
  // Issue requests for phrase internal ngrams.
  // Sync if necessary. (or autosync).
  const_cast<LanguageModelLDHT*>(this)->sync();

  // TODO(wilson): handle nonterminal words.
  LDHT::Client* client = getClientUnsafe();
  // Score the first order - 1 words of the phrase.
  int order = LDHT::NewNgram::k_max_order;
  int prefix_start = 0;
  int prefix_end = std::min(phrase.GetSize(), static_cast<size_t>(order - 1));
  LDHT::NewNgram ngram;
  std::deque<int> full_score_tags;
  for (int word_idx = prefix_start; word_idx < prefix_end; ++word_idx) {
    ngram.appendGram(phrase.GetWord(word_idx)
                     .GetFactor(m_factorType)->GetString().c_str());
    full_score_tags.push_back(client->requestNgram(ngram));
  }
  // Now score all subsequent ngrams to end of phrase.
  int internal_start = prefix_end;
  int internal_end = phrase.GetSize();
  std::deque<int> internal_score_tags;
  for (int word_idx = internal_start; word_idx < internal_end; ++word_idx) {
    ngram.appendGram(phrase.GetWord(word_idx)
                     .GetFactor(m_factorType)->GetString().c_str());
    internal_score_tags.push_back(client->requestNgram(ngram));
  }

  // Wait for resposes from the servers.
  //client->awaitResponses();

  // Calculate the full phrase score, and the internal score.
  fullScore = 0.0;
  while (!full_score_tags.empty()) {
    fullScore += client->getNgramScore(full_score_tags.front());
    full_score_tags.pop_front();
  }
  ngramScore = 0.0;
  while (!internal_score_tags.empty()) {
    float score = client->getNgramScore(internal_score_tags.front());
    internal_score_tags.pop_front();
    fullScore += score;
    ngramScore += score;
  }
  fullScore = TransformLMScore(fullScore);
  ngramScore = TransformLMScore(ngramScore);
  oovCount = 0;
}

void LanguageModelLDHT::IssueRequestsFor(Hypothesis& hypo,
    const FFState* input_state)
{
  // TODO(wilson): handle nonterminal words.
  LDHT::Client* client = getClientUnsafe();

  // Create a new state and copy the contents of the input_state if
  // supplied.
  LDHTLMState* new_state = new LDHTLMState();
  if (input_state == NULL) {
    if (hypo.GetCurrTargetWordsRange().GetStartPos() != 0) {
      UTIL_THROW2("got a null state but not at start of sentence");
    }
    new_state->gram_fingerprints.appendGram(BOS_);
  } else {
    if (hypo.GetCurrTargetWordsRange().GetStartPos() == 0) {
      UTIL_THROW2("got a non null state but at start of sentence");
    }
    new_state->copyFrom(static_cast<const LDHTLMState&>(*input_state));
  }

  // Score ngrams that overlap with the previous phrase.
  int order = LDHT::NewNgram::k_max_order;
  int phrase_start = hypo.GetCurrTargetWordsRange().GetStartPos();
  int phrase_end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  int overlap_start = phrase_start;
  int overlap_end = std::min(phrase_end, phrase_start + order - 1);
  int word_idx = overlap_start;
  LDHT::NewNgram& ngram = new_state->gram_fingerprints;
  for (; word_idx < overlap_end; ++word_idx) {
    ngram.appendGram(
      hypo.GetFactor(word_idx, m_factorType)->GetString().c_str());
    new_state->appendRequestTag(client->requestNgram(ngram));
  }
  // No need to score phrase internal ngrams, but keep track of them
  // in the state (which in this case is the NewNgram containing the
  // hashes of the individual grams).
  for (; word_idx < phrase_end; ++word_idx) {
    ngram.appendGram(
      hypo.GetFactor(word_idx, m_factorType)->GetString().c_str());
  }
  // If this is the last phrase in the sentence, score the last ngram
  // with the end of sentence marker on it.
  if (hypo.IsSourceCompleted()) {
    ngram.appendGram(EOS_);
    //request_tags.push_back(client->requestNgram(ngram));
    new_state->appendRequestTag(client->requestNgram(ngram));
  }
  hypo.SetFFState(m_state_idx, new_state);
}

void LanguageModelLDHT::sync()
{
  m_calc_score_count = 0;
  getClientUnsafe()->awaitResponses();
}

void LanguageModelLDHT::SetFFStateIdx(int state_idx)
{
  m_state_idx = state_idx;
}

FFState* LanguageModelLDHT::Evaluate(
  const Hypothesis& hypo,
  const FFState* input_state_ignored,
  ScoreComponentCollection* score_output) const
{
  // Input state is the state from the previous hypothesis, which
  // we are not interested in. The requests for this hypo should
  // already have been issued via IssueRequestsFor() and the LM then
  // synced and all responses processed, and the tags placed in our
  // FFState of hypo.
  LDHTLMState* state = const_cast<LDHTLMState*>(static_cast<const LDHTLMState*>(hypo.GetFFState(m_state_idx)));

  float score = calcScoreFromState(state);
  score = FloorScore(TransformLMScore(score));
  score_output->PlusEquals(this, score);

  return state;
}

FFState* LanguageModelLDHT::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  int featureID,
  ScoreComponentCollection* accumulator) const
{
  return NULL;
}

float LanguageModelLDHT::calcScoreFromState(LDHTLMState* state) const
{
  float score = 0.0;
  std::vector<int>::iterator tag_iter;
  LDHT::Client* client = getClientUnsafe();
  for (tag_iter = state->requestTagsBegin();
       tag_iter != state->requestTagsEnd();
       ++tag_iter) {
    score += client->getNgramScore(*tag_iter);
  }
  state->clearRequestTags();
  state->setFinalised();
  return score;
}

}  // namespace Moses.

#endif
