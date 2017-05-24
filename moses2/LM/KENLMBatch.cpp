/*
 * KENLMBatch.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include <vector>

#ifdef _linux
#include <pthread.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "KENLMBatch.h"
#include "../Phrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "lm/state.hh"
#include "lm/left.hh"
#include "util/exception.hh"
#include "util/tokenize_piece.hh"
#include "util/string_stream.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

namespace Moses2
{

struct KenLMState: public FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const KenLMState &other = static_cast<const KenLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }

  virtual std::string ToString() const {
    stringstream ss;
    for (size_t i = 0; i < state.Length(); ++i) {
      ss << state.words[i] << " ";
    }
    return ss.str();
  }

};

/////////////////////////////////////////////////////////////////
class MappingBuilder: public lm::EnumerateVocab
{
public:
  MappingBuilder(FactorCollection &factorCollection, System &system,
                 std::vector<lm::WordIndex> &mapping) :
    m_factorCollection(factorCollection), m_system(system), m_mapping(mapping) {
  }

  void Add(lm::WordIndex index, const StringPiece &str) {
    std::size_t factorId = m_factorCollection.AddFactor(str, m_system, false)->GetId();
    if (m_mapping.size() <= factorId) {
      // 0 is <unk> :-)
      m_mapping.resize(factorId + 1);
    }
    m_mapping[factorId] = index;
  }

private:
  FactorCollection &m_factorCollection;
  std::vector<lm::WordIndex> &m_mapping;
  System &m_system;
};

/////////////////////////////////////////////////////////////////
KENLMBatch::KENLMBatch(size_t startInd, const std::string &line)
  :StatefulFeatureFunction(startInd, line)
  ,m_numHypos(0)
{
  cerr << "KENLMBatch::KENLMBatch" << endl;
  ReadParameters();
}

KENLMBatch::~KENLMBatch()
{
  // TODO Auto-generated destructor stub
}

void KENLMBatch::Load(System &system)
{
  cerr << "KENLMBatch::Load" << endl;
  FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor(BOS_, system, false);
  m_eos = fc.AddFactor(EOS_, system, false);

  lm::ngram::Config config;
  config.messages = NULL;

  FactorCollection &collection = system.GetVocab();
  MappingBuilder builder(collection, system, m_lmIdLookup);
  config.enumerate_vocab = &builder;
  config.load_method = m_load_method;

  m_ngram.reset(new Model(m_path.c_str(), config));
}

FFState* KENLMBatch::BlankState(MemPool &pool, const System &sys) const
{
  KenLMState *ret = new (pool.Allocate<KenLMState>()) KenLMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void KENLMBatch::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                      const InputType &input, const Hypothesis &hypo) const
{
  KenLMState &stateCast = static_cast<KenLMState&>(state);
  stateCast.state = m_ngram->BeginSentenceState();
}

void KENLMBatch::EvaluateInIsolation(MemPool &pool, const System &system,
                                     const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
                                     SCORE &estimatedScore) const
{
  // contains factors used by this LM
  float fullScore, nGramScore;
  size_t oovCount;

  CalcScore(targetPhrase, fullScore, nGramScore, oovCount);

  float estimateScore = fullScore - nGramScore;

  bool GetLMEnableOOVFeature = false;
  if (GetLMEnableOOVFeature) {
    float scoresVec[2], estimateScoresVec[2];
    scoresVec[0] = nGramScore;
    scoresVec[1] = oovCount;
    scores.PlusEquals(system, *this, scoresVec);

    estimateScoresVec[0] = estimateScore;
    estimateScoresVec[1] = 0;
    SCORE weightedScore = Scores::CalcWeightedScore(system, *this,
                          estimateScoresVec);
    estimatedScore += weightedScore;
  } else {
    scores.PlusEquals(system, *this, nGramScore);

    SCORE weightedScore = Scores::CalcWeightedScore(system, *this,
                          estimateScore);
    estimatedScore += weightedScore;
  }
}

void KENLMBatch::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                     const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                     SCORE &estimatedScore) const
{
}

void KENLMBatch::EvaluateWhenApplied(const ManagerBase &mgr,
                                     const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                     FFState &state) const
{
  KenLMState &stateCast = static_cast<KenLMState&>(state);

  const System &system = mgr.system;

  const lm::ngram::State &in_state =
    static_cast<const KenLMState&>(prevState).state;

  if (!hypo.GetTargetPhrase().GetSize()) {
    stateCast.state = in_state;
    return;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_ngram->Order() - 1);

  std::size_t position = begin;
  Model::State aux_state;
  Model::State *state0 = &stateCast.state, *state1 = &aux_state;

  float score = m_ngram->Score(in_state, TranslateID(hypo.GetWord(position)),
                               *state0);
  ++position;
  for (; position < adjust_end; ++position) {
    score += m_ngram->Score(*state0, TranslateID(hypo.GetWord(position)),
                            *state1);
    std::swap(state0, state1);
  }

  if (hypo.GetBitmap().IsComplete()) {
    // Score end of sentence.
    std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
    const lm::WordIndex *last = LastIDs(hypo, &indices.front());
    score += m_ngram->FullScoreForgotState(&indices.front(), last,
                                           m_ngram->GetVocabulary().EndSentence(), stateCast.state).prob;
  } else if (adjust_end < end) {
    // Get state after adding a long phrase.
    std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
    const lm::WordIndex *last = LastIDs(hypo, &indices.front());
    m_ngram->GetState(&indices.front(), last, stateCast.state);
  } else if (state0 != &stateCast.state) {
    // Short enough phrase that we can just reuse the state.
    stateCast.state = *state0;
  }

  score = TransformLMScore(score);

  bool OOVFeatureEnabled = false;
  if (OOVFeatureEnabled) {
    std::vector<float> scoresVec(2);
    scoresVec[0] = score;
    scoresVec[1] = 0.0;
    scores.PlusEquals(system, *this, scoresVec);
  } else {
    scores.PlusEquals(system, *this, score);
  }
}

void KENLMBatch::CalcScore(const Phrase<Moses2::Word> &phrase, float &fullScore,
                           float &ngramScore, std::size_t &oovCount) const
{
  fullScore = 0;
  ngramScore = 0;
  oovCount = 0;

  if (!phrase.GetSize()) return;

  lm::ngram::ChartState discarded_sadly;
  lm::ngram::RuleScore<Model> scorer(*m_ngram, discarded_sadly);

  size_t position;
  if (m_bos == phrase[0][m_factorType]) {
    scorer.BeginSentence();
    position = 1;
  } else {
    position = 0;
  }

  size_t ngramBoundary = m_ngram->Order() - 1;

  size_t end_loop = std::min(ngramBoundary, phrase.GetSize());
  for (; position < end_loop; ++position) {
    const Word &word = phrase[position];
    lm::WordIndex index = TranslateID(word);
    scorer.Terminal(index);
    if (!index) ++oovCount;
  }
  float before_boundary = fullScore + scorer.Finish();
  for (; position < phrase.GetSize(); ++position) {
    const Word &word = phrase[position];
    lm::WordIndex index = TranslateID(word);
    scorer.Terminal(index);
    if (!index) ++oovCount;
  }
  fullScore += scorer.Finish();

  ngramScore = TransformLMScore(fullScore - before_boundary);
  fullScore = TransformLMScore(fullScore);
}

// Convert last words of hypothesis into vocab ids, returning an end pointer.
lm::WordIndex *KENLMBatch::LastIDs(const Hypothesis &hypo,
                                   lm::WordIndex *indices) const
{
  lm::WordIndex *index = indices;
  lm::WordIndex *end = indices + m_ngram->Order() - 1;
  int position = hypo.GetCurrTargetWordsRange().GetEndPos();
  for (;; ++index, --position) {
    if (index == end) return index;
    if (position == -1) {
      *index = m_ngram->GetVocabulary().BeginSentence();
      return index + 1;
    }
    *index = TranslateID(hypo.GetWord(position));
  }
}

void KENLMBatch::SetParameter(const std::string& key,
                              const std::string& value)
{
  //cerr << "key=" << key << " " << value << endl;
  if (key == "path") {
    m_path = value;
  } else if (key == "order") {
    // ignore
  } else if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "lazyken") {
    m_load_method =
      boost::lexical_cast<bool>(value) ?
      util::LAZY : util::POPULATE_OR_READ;
  } else if (key == "load") {
    if (value == "lazy") {
      m_load_method = util::LAZY;
    } else if (value == "populate_or_lazy") {
      m_load_method = util::POPULATE_OR_LAZY;
    } else if (value == "populate_or_read" || value == "populate") {
      m_load_method = util::POPULATE_OR_READ;
    } else if (value == "read") {
      m_load_method = util::READ;
    } else if (value == "parallel_read") {
      m_load_method = util::PARALLEL_READ;
    } else {
      UTIL_THROW2("Unknown KenLM load method " << value);
    }
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

  //cerr << "SetParameter done" << endl;
}

void KENLMBatch::EvaluateWhenAppliedBatch(
  const Batch &batch) const
{
  {
    // write lock
    boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
    m_batches.push_back(&batch);
    m_numHypos += batch.size();
  }
  //cerr << "m_numHypos=" << m_numHypos << endl;

  if (m_numHypos > 0) {
    // process batch
    EvaluateWhenAppliedBatch();

    m_batches.clear();
    m_numHypos = 0;

    m_threadNeeded.notify_all();
  } else {
    boost::mutex::scoped_lock lock(m_mutex);
    m_threadNeeded.wait(lock);
  }
}

void KENLMBatch::EvaluateWhenAppliedBatch() const
{
  BOOST_FOREACH(const Batch *batch, m_batches) {
    //cerr << "batch=" << batch->size() << endl;
    BOOST_FOREACH(Hypothesis *hypo, *batch) {
      hypo->EvaluateWhenApplied(*this);
    }
  }
}

void KENLMBatch::EvaluateWhenApplied(const SCFG::Manager &mgr,
                                     const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                     FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

}

