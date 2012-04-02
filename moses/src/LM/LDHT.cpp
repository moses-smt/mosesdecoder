//
// Oliver Wilson <oliver.wilson@ed.ac.uk>
//

#include "LM/Base.h"
#include "LM/LDHT.h"
#include "../FFState.h"
#include "../TypeDef.h"
#include "../Hypothesis.h"

#include <LDHT/Client.h>
#include <LDHT/ClientLocal.h>
#include <LDHT/NewNgram.h>
#include <LDHT/FactoryCollection.h>

#include <boost/thread/tss.hpp>

namespace Moses {

struct LDHTLMState : public FFState {
  LDHT::NewNgram gram_fingerprints;

  int Compare(const FFState& uncast_other) const {
    const LDHTLMState &other = static_cast<const LDHTLMState&>(uncast_other);
    return gram_fingerprints.compareMoses(other.gram_fingerprints);
  }

  void copyFrom(const LDHTLMState& other) {
    gram_fingerprints.copyFrom(other.gram_fingerprints);
  }
};

class LanguageModelLDHT : public LanguageModel {
public:
    LanguageModelLDHT();
    LanguageModelLDHT(const std::string& path,
                      ScoreIndexManager& manager,
                      FactorType factorType);
    LanguageModelLDHT(ScoreIndexManager& manager,
                      LanguageModelLDHT& copyFrom);
    std::string GetScoreProducerDescription(unsigned) const {
      std::ostringstream oss;
      oss << "LM_" << LDHT::NewNgram::k_max_order << "gram";
      return oss.str();
    }
    LDHT::Client* getClientUnsafe() const;
    LDHT::Client* getClientSafe();
    LDHT::Client* initTSSClient();
    virtual ~LanguageModelLDHT();
    virtual LanguageModel* Duplicate(
            ScoreIndexManager& scoreIndexManager) const;
    virtual void InitializeBeforeSentenceProcessing();
    virtual void CleanUpAfterSentenceProcessing();
    virtual const FFState* EmptyHypothesisState(const InputType& input) const;
    virtual bool Useable(const Phrase& phrase) const;
    virtual void CalcScore(const Phrase& phrase,
                           float& fullScore,
                           float& ngramScore,
                           std::size_t& oovCount) const;
    FFState* Evaluate(const Hypothesis& hypo,
                      const FFState* input_state,
                      ScoreComponentCollection* score_output) const;
    FFState* EvaluateChart(const ChartHypothesis& hypo,
                           int featureID,
                           ScoreComponentCollection* accumulator) const;

protected:
    boost::thread_specific_ptr<LDHT::Client> m_client;
    std::string m_configPath;
    FactorType m_factorType;

};

LanguageModel* ConstructLDHTLM(const std::string& path,
                               ScoreIndexManager& manager,
                               FactorType factorType) {
    return new LanguageModelLDHT(path, manager, factorType);
}

LanguageModelLDHT::LanguageModelLDHT() : LanguageModel(), m_client(NULL) {
    m_enableOOVFeature = false;
}

LanguageModelLDHT::LanguageModelLDHT(ScoreIndexManager& manager,
                                     LanguageModelLDHT& copyFrom) {
    //m_client = copyFrom.m_client;
    m_factorType = copyFrom.m_factorType;
    m_configPath = copyFrom.m_configPath;
    Init(manager);
}

LanguageModelLDHT::LanguageModelLDHT(const std::string& path,
                                     ScoreIndexManager& manager,
                                     FactorType factorType)
                                            : m_factorType(factorType) {
    m_configPath = path;
    Init(manager);
}

LanguageModelLDHT::~LanguageModelLDHT() {
    // TODO(wilson): should cleanup for each individual thread.
    delete getClientSafe();
}

LanguageModel* LanguageModelLDHT::Duplicate(
            ScoreIndexManager& scoreIndexManager) const {
    return NULL;
}

// Check that there is a TSS Client instance, and instantiate one if
// there isn't.
LDHT::Client* LanguageModelLDHT::getClientSafe() {
    if (m_client.get() == NULL)
        m_client.reset(initTSSClient());
    return m_client.get();
}

// Do not check that there is a TSS Client instance.
LDHT::Client* LanguageModelLDHT::getClientUnsafe() const {
    return m_client.get();
}

LDHT::Client* LanguageModelLDHT::initTSSClient() {
    std::ifstream config_file(m_configPath.c_str());
    std::string ldht_config_path;
    getline(config_file, ldht_config_path);
    std::string ldhtlm_config_path;
    getline(config_file, ldhtlm_config_path);

    LDHT::FactoryCollection* factory_collection =
            LDHT::FactoryCollection::createDefaultFactoryCollection();

    LDHT::Client* client;
    client = new LDHT::ClientLocal();
    //client = new LDHT::Client();
    client->fromXmlFiles(*factory_collection,
                           ldht_config_path,
                           ldhtlm_config_path);
    return client;
}

void LanguageModelLDHT::InitializeBeforeSentenceProcessing() {
    getClientSafe()->clearCache();
}

void LanguageModelLDHT::CleanUpAfterSentenceProcessing() {
}

const FFState* LanguageModelLDHT::EmptyHypothesisState(
        const InputType& input) const {
    return NULL;
}

bool LanguageModelLDHT::Useable(const Phrase& phrase) const {
    return (phrase.GetSize() > 0 && phrase.GetFactor(0, m_factorType) != NULL);
}

void LanguageModelLDHT::CalcScore(const Phrase& phrase,
                                  float& fullScore,
                                  float& ngramScore,
                                  std::size_t& oovCount) const {
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
    client->awaitResponses();

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

FFState* LanguageModelLDHT::Evaluate(
        const Hypothesis& hypo,
        const FFState* input_state,
        ScoreComponentCollection* score_output) const {
    // TODO(wilson): handle nonterminal words.
    LDHT::Client* client = getClientUnsafe();

    // Create a new state and copy the contents of the input_state if
    // supplied.
    LDHTLMState* new_state = new LDHTLMState();
    if (input_state == NULL) {
        if (hypo.GetCurrTargetWordsRange().GetStartPos() != 0) {
            V("got a null state but not at start of sentence");
            abort();
        }
        new_state->gram_fingerprints.appendGram(BOS_);
    }
    else {
        if (hypo.GetCurrTargetWordsRange().GetStartPos() == 0) {
            V("got a non null state but at start of sentence");
            abort();
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
    std::deque<int> request_tags;
    for (; word_idx < overlap_end; ++word_idx) {
        ngram.appendGram(
                hypo.GetFactor(word_idx, m_factorType)->GetString().c_str());
        request_tags.push_back(client->requestNgram(ngram));
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
        request_tags.push_back(client->requestNgram(ngram));
    }
    // Await responses from the server.
    client->awaitResponses();

    // Calculate scores given the request tags.
    float score = 0;
    while (!request_tags.empty()) {
        score += client->getNgramScore(request_tags.front());
        request_tags.pop_front();
    }

    score = FloorScore(TransformLMScore(score));
    score_output->PlusEquals(this, score);

    return new_state;
}

FFState* LanguageModelLDHT::EvaluateChart(
        const ChartHypothesis& hypo,
        int featureID,
        ScoreComponentCollection* accumulator) const {
    return NULL;
}

}  // namespace Moses.

