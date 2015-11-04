/*
 * KENLM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */

#include <vector>
#include "KENLM.h"
#include "../TargetPhrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../Search/Hypothesis.h"
#include "../Search/Manager.h"
#include "lm/state.hh"
#include "lm/left.hh"
#include "moses/FactorCollection.h"

using namespace std;

struct KenLMState : public Moses::FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const Moses::FFState& o) const {
    const KenLMState &other = static_cast<const KenLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }

};

/////////////////////////////////////////////////////////////////
class MappingBuilder : public lm::EnumerateVocab
{
public:
  MappingBuilder(Moses::FactorCollection &factorCollection, std::vector<lm::WordIndex> &mapping)
    : m_factorCollection(factorCollection), m_mapping(mapping) {}

  void Add(lm::WordIndex index, const StringPiece &str) {
    std::size_t factorId = m_factorCollection.AddFactor(str)->GetId();
    if (m_mapping.size() <= factorId) {
      // 0 is <unk> :-)
      m_mapping.resize(factorId + 1);
    }
    m_mapping[factorId] = index;
  }

private:
  Moses::FactorCollection &m_factorCollection;
  std::vector<lm::WordIndex> &m_mapping;
};

/////////////////////////////////////////////////////////////////
KENLM::KENLM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
	ReadParameters();
}

KENLM::~KENLM()
{
	// TODO Auto-generated destructor stub
}

void KENLM::Load(System &system)
{
  Moses::FactorCollection &fc = system.GetVocab();

  m_bos = fc.AddFactor("<s>", false);
  m_eos = fc.AddFactor("</s>", false);

  lm::ngram::Config config;
  config.messages = NULL;

  Moses::FactorCollection &collection = system.GetVocab();
  MappingBuilder builder(collection, m_lmIdLookup);
  config.enumerate_vocab = &builder;
  //config.load_method = lazy ? util::LAZY : util::POPULATE_OR_READ;

  m_ngram.reset(new Model(m_path.c_str(), config));
}

void KENLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
	  m_path = value;
  }
  else if (key == "factor") {
	  m_factorType = Moses::Scan<Moses::FactorType>(value);
  }
  else if (key == "order") {
	  // don't need to store it
  }
  else {
	  StatefulFeatureFunction::SetParameter(key, value);
  }
}

//! return the state associated with the empty hypothesis for a given sentence
const Moses::FFState* KENLM::EmptyHypothesisState(const Manager &mgr, const PhraseImpl &input) const
{
  MemPool &pool = mgr.GetPool();
  KenLMState *ret = new (pool.Allocate<KenLMState>()) KenLMState();
  ret->state = m_ngram->BeginSentenceState();
  return ret;
}

void
KENLM::EvaluateInIsolation(const System &system,
		  const Phrase &source, const TargetPhrase &targetPhrase,
        Scores &scores,
        Scores *estimatedScores) const
{
  // contains factors used by this LM
  float fullScore, nGramScore;
  size_t oovCount;

  CalcScore(targetPhrase, fullScore, nGramScore, oovCount);

  float estimateScore = fullScore - nGramScore;

  bool GetLMEnableOOVFeature = false;
  if (GetLMEnableOOVFeature) {
	vector<float> scoresVec(2), estimateScoresVec(2);
	scoresVec[0] = nGramScore;
	scoresVec[1] = oovCount;
	scores.Assign(system, *this, scoresVec);

	estimateScoresVec[0] = estimateScore;
	estimateScoresVec[1] = 0;
	estimatedScores->Assign(system, *this, estimateScoresVec);
  }
  else {
	scores.Assign(system, *this, nGramScore);
	estimatedScores->Assign(system, *this, estimateScore);
  }
}

Moses::FFState* KENLM::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const Moses::FFState &prevState,
  Scores &scores) const
{
  const System &system = mgr.GetSystem();
  MemPool &pool = mgr.GetPool();

  const lm::ngram::State &in_state = static_cast<const KenLMState&>(prevState).state;

  KenLMState *ret = new (pool.Allocate<KenLMState>()) KenLMState();

  if (!hypo.GetTargetPhrase().GetSize()) {
	ret->state = in_state;
	return ret;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_ngram->Order() - 1);

  std::size_t position = begin;
  typename Model::State aux_state;
  typename Model::State *state0 = &ret->state, *state1 = &aux_state;

  float score = m_ngram->Score(in_state, TranslateID(hypo.GetWord(position)), *state0);
  ++position;
  for (; position < adjust_end; ++position) {
	score += m_ngram->Score(*state0, TranslateID(hypo.GetWord(position)), *state1);
	std::swap(state0, state1);
  }

  if (hypo.GetBitmap().IsComplete()) {
	// Score end of sentence.
	std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
	const lm::WordIndex *last = LastIDs(hypo, &indices.front());
	score += m_ngram->FullScoreForgotState(&indices.front(), last, m_ngram->GetVocabulary().EndSentence(), ret->state).prob;
  } else if (adjust_end < end) {
	// Get state after adding a long phrase.
	std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
	const lm::WordIndex *last = LastIDs(hypo, &indices.front());
	m_ngram->GetState(&indices.front(), last, ret->state);
  } else if (state0 != &ret->state) {
	// Short enough phrase that we can just reuse the state.
	ret->state = *state0;
  }

  score = Moses::TransformLMScore(score);

  bool OOVFeatureEnabled = false;
  if (OOVFeatureEnabled) {
	std::vector<float> scoresVec(2);
	scoresVec[0] = score;
	scoresVec[1] = 0.0;
	scores.PlusEquals(system, *this, scoresVec);
  } else {
	scores.PlusEquals(system, *this, score);
  }

  return ret;
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

	  ngramScore = Moses::TransformLMScore(fullScore - before_boundary);
	  fullScore = Moses::TransformLMScore(fullScore);
}

lm::WordIndex KENLM::TranslateID(const Word &word) const {
  std::size_t factor = word[m_factorType]->GetId();
  return (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
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
