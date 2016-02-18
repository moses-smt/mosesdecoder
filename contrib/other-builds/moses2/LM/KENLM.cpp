/*
 * KENLM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <sstream>
#include <vector>
#include "KENLM.h"
#include "../TargetPhrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../Search/Hypothesis.h"
#include "../Search/Manager.h"
#include "lm/state.hh"
#include "lm/left.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

namespace Moses2
{

struct KenLMState : public FFState {
  const lm::ngram::State *state;
  virtual size_t hash() const {
    size_t ret = hash_value(*state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const KenLMState &other = static_cast<const KenLMState &>(o);
    bool ret = *state == *other.state;
    return ret;
  }

  virtual std::string ToString() const
  {
	  stringstream ss;
	  for (size_t i = 0; i < state->Length(); ++i) {
		  ss << state->words[i] << " ";
	  }
	  return ss.str();
  }

};

/////////////////////////////////////////////////////////////////
class MappingBuilder : public lm::EnumerateVocab
{
public:
  MappingBuilder(FactorCollection &factorCollection, System &system, std::vector<lm::WordIndex> &mapping)
    : m_factorCollection(factorCollection)
	, m_system(system)
	, m_mapping(mapping)
  {}

  void Add(lm::WordIndex index, const StringPiece &str) {
	std::size_t factorId = m_factorCollection.AddFactor(str, m_system)->GetId();
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
KENLM::KENLM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
,m_lazy(false)
{
	ReadParameters();
}

KENLM::~KENLM()
{
	// TODO Auto-generated destructor stub
}

void KENLM::Load(System &system)
{
  FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor(BOS_, system, false);
  m_eos = fc.AddFactor(EOS_, system, false);

  lm::ngram::Config config;
  config.messages = NULL;

  FactorCollection &collection = system.GetVocab();
  MappingBuilder builder(collection, system, m_lmIdLookup);
  config.enumerate_vocab = &builder;
  config.load_method = m_lazy ? util::LAZY : util::POPULATE_OR_READ;

  m_ngram.reset(new Model(m_path.c_str(), config));
}

void KENLM::InitializeForInput(const Manager &mgr) const
{
	CacheColl &cache = GetCache();
	cache.clear();
	mgr.lmCache = &cache;
}

// clean up temporary memory, called after processing each sentence
void KENLM::CleanUpAfterSentenceProcessing(const Manager &mgr) const
{
}

void KENLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Scan<FactorType>(value);
  }
  else if (key == "lazyken") {
	  m_lazy = Scan<bool>(value);
  }
  else if (key == "order") {
	  // don't need to store it
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* KENLM::BlankState(MemPool &pool) const
{
  KenLMState *ret = new (pool.Allocate<KenLMState>()) KenLMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void KENLM::EmptyHypothesisState(FFState &state,
		const Manager &mgr,
		const InputType &input,
		const Hypothesis &hypo) const
{
  KenLMState &stateCast = static_cast<KenLMState&>(state);
  stateCast.state = &m_ngram->BeginSentenceState();
}

void
KENLM::EvaluateInIsolation(MemPool &pool,
		const System &system,
		const Phrase &source,
		const TargetPhrase &targetPhrase,
        Scores &scores,
		SCORE *estimatedScore) const
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
	SCORE weightedScore = Scores::CalcWeightedScore(system, *this, estimateScoresVec);
	(*estimatedScore) += weightedScore;
  }
  else {
	scores.PlusEquals(system, *this, nGramScore);

	SCORE weightedScore = Scores::CalcWeightedScore(system, *this, estimateScore);
	(*estimatedScore) += weightedScore;
  }
}

void KENLM::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const FFState &prevState,
  Scores &scores,
  FFState &state) const
{
  MemPool &pool = mgr.GetPool();
  KenLMState &stateCast = static_cast<KenLMState&>(state);

  const System &system = mgr.system;

  const lm::ngram::State *in_state = static_cast<const KenLMState&>(prevState).state;

  if (!hypo.GetTargetPhrase().GetSize()) {
    stateCast.state = in_state;
	return;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_ngram->Order() - 1);

  std::size_t position = begin;
  typename Model::State aux_state;
  const Model::State *state0 = stateCast.state;
  const Model::State *state1 = &aux_state;

  const LMCacheValue &val = ScoreAndCache(mgr, *in_state, TranslateID(hypo.GetWord(position)));
  float score = val.first;
  state0 = val.second;

  ++position;
  for (; position < adjust_end; ++position) {
	const LMCacheValue &val = ScoreAndCache(mgr, *state0, TranslateID(hypo.GetWord(position)));
	score += val.first;
	state1 = val.second;

	std::swap(state0, state1);
  }

  if (hypo.GetBitmap().IsComplete()) {
	// Score end of sentence.
	std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
	const lm::WordIndex *last = LastIDs(hypo, &indices.front());
	lm::ngram::State *newState = new (pool.Allocate<lm::ngram::State>()) lm::ngram::State();

	score += m_ngram->FullScoreForgotState(&indices.front(), last, m_ngram->GetVocabulary().EndSentence(), *newState).prob;
	stateCast.state = newState;
  }
  else if (adjust_end < end) {
	// Get state after adding a long phrase.
	std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
	const lm::WordIndex *last = LastIDs(hypo, &indices.front());
	lm::ngram::State *newState = new (pool.Allocate<lm::ngram::State>()) lm::ngram::State();

	m_ngram->GetState(&indices.front(), last, *newState);
	stateCast.state = newState;
  }
  else {
	// Short enough phrase that we can just reuse the state.
	  stateCast.state = state0;
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

void KENLM::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const
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
lm::WordIndex *KENLM::LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const {
  lm::WordIndex *index = indices;
  lm::WordIndex *end = indices + m_ngram->Order() - 1;
  int position = hypo.GetCurrTargetWordsRange().GetEndPos();
  for (; ; ++index, --position) {
    if (index == end) return index;
    if (position == -1) {
      *index = m_ngram->GetVocabulary().BeginSentence();
      return index + 1;
    }
    *index = TranslateID(hypo.GetWord(position));
  }
}

const KENLM::LMCacheValue &KENLM::ScoreAndCache(const Manager &mgr, const lm::ngram::State &in_state, const lm::WordIndex new_word) const
{
	MemPool &pool = mgr.GetPool();
	//cerr << "score=";
	LMCacheValue *val;

	CacheColl &lmCache = *((CacheColl*)mgr.lmCache);
	LMCacheKey key(in_state, new_word);
	CacheColl::iterator iter;
	iter = lmCache.find(key);
	if (iter == lmCache.end()) {
		lm::ngram::State *newState = new (pool.Allocate<lm::ngram::State>()) lm::ngram::State();
		float score = m_ngram->Score(in_state, new_word, *newState);

		val = &lmCache[key];
		val->first = score;
		val->second = newState;
	}
	else {
		val = &iter->second;
	}

	//cerr << score << " " << (int) out_state.length << endl;
	return *val;
}

KENLM::CacheColl &KENLM::GetCache() const
{
	return GetThreadSpecificObj(m_cache);
}

}

