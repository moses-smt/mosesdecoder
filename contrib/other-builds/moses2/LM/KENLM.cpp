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
  lm::ngram::ChartState state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const KenLMState &other = static_cast<const KenLMState &>(o);
    bool ret = state == other.state;
    return ret;
  }

  virtual std::string ToString() const
  {
	  /*
	  stringstream ss;
	  for (size_t i = 0; i < state.Length(); ++i) {
		  ss << state.words[i] << " ";
	  }
	  return ss.str();
	  */
	  return "KenLMState";
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
  lm::ngram::RuleScore<Model> scorer(*m_ngram, stateCast.state);
  scorer.BeginSentence();
  scorer.Finish();
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

  lm::ngram::ChartState *state = new (pool.Allocate<lm::ngram::ChartState>()) lm::ngram::ChartState();
  CalcScore(targetPhrase, fullScore, nGramScore, oovCount, *state);
  targetPhrase.chartState = (void*) state;

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
  const System &system = mgr.system;

  const KenLMState &prevStateCast = static_cast<const KenLMState&>(prevState);
  KenLMState &stateCast = static_cast<KenLMState&>(state);

  const lm::ngram::ChartState &prevKenState = prevStateCast.state;
  lm::ngram::ChartState &kenState = stateCast.state;

  const TargetPhrase &tp = hypo.GetTargetPhrase();
  size_t tpSize = tp.GetSize();
  if (!tpSize) {
    stateCast.state = prevKenState;
	return;
  }

  // NEW CODE - start
  //const lm::ngram::ChartState &chartStateInIsolation = *static_cast<const lm::ngram::ChartState*>(tp.chartState);
  lm::ngram::RuleScore<Model> ruleScore(*m_ngram, kenState);
  ruleScore.NonTerminal(prevKenState, 0);

  // each word in new tp
  for (size_t i = 0; i < tpSize; ++i) {
	  const Word &word = tp[i];
	  lm::WordIndex lmInd = TranslateID(word);
	  ruleScore.Terminal(lmInd);
  }

  if (hypo.GetBitmap().IsComplete()) {
	  ruleScore.Terminal(m_ngram->GetVocabulary().EndSentence());
  }
  // NEW CODE - end


  float score10 = ruleScore.Finish();
  float score = TransformLMScore(score10);

  /*
  stringstream strme;
  hypo.OutputToStream(strme);
  cerr << "HELLO " << score10 << " " << score << " " << strme.str() << " " << hypo.GetBitmap() << endl;
  */

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

void KENLM::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount, lm::ngram::ChartState &state) const
{
	  fullScore = 0;
	  ngramScore = 0;
	  oovCount = 0;

	  if (!phrase.GetSize()) return;

	  lm::ngram::RuleScore<Model> scorer(*m_ngram, state);

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

}

