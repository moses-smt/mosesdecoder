/*
 * KENLMBatch.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */

#include <vector>
#include "KENLMBatch.h"
#include "../TargetPhrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../Search/Hypothesis.h"
#include "../Search/Manager.h"
#include "lm/state.hh"
#include "lm/left.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

struct KENLMBatchState : public FFState {
  lm::ngram::State state;
  virtual size_t hash() const {
    size_t ret = hash_value(state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const KENLMBatchState &other = static_cast<const KENLMBatchState &>(o);
    bool ret = state == other.state;
    return ret;
  }

};

/////////////////////////////////////////////////////////////////
class MappingBuilder : public lm::EnumerateVocab
{
public:
  MappingBuilder(FactorCollection &factorCollection, std::vector<lm::WordIndex> &mapping)
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
  FactorCollection &m_factorCollection;
  std::vector<lm::WordIndex> &m_mapping;
};

/////////////////////////////////////////////////////////////////
KENLMBatch::KENLMBatch(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
,m_lazy(false)
{
	ReadParameters();
}

KENLMBatch::~KENLMBatch()
{
	// TODO Auto-generated destructor stub
}

void KENLMBatch::Load(System &system)
{
  FactorCollection &fc = system.vocab;

  m_bos = fc.AddFactor("<s>", false);
  m_eos = fc.AddFactor("</s>", false);

  lm::ngram::Config config;
  config.messages = NULL;

  FactorCollection &collection = system.vocab;
  MappingBuilder builder(collection, m_lmIdLookup);
  config.enumerate_vocab = &builder;
  config.load_method = m_lazy ? util::LAZY : util::POPULATE_OR_READ;

  m_ngram.reset(new Model(m_path.c_str(), config));
}

void KENLMBatch::SetParameter(const std::string& key, const std::string& value)
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

FFState* KENLMBatch::BlankState(const Manager &mgr, const PhraseImpl &input) const
{
  MemPool &pool = mgr.GetPool();
  KENLMBatchState *ret = new (pool.Allocate<KENLMBatchState>()) KENLMBatchState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void KENLMBatch::EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const
{
  KENLMBatchState &stateCast = static_cast<KENLMBatchState&>(state);
  stateCast.state = m_ngram->BeginSentenceState();
}

void
KENLMBatch::EvaluateInIsolation(const System &system,
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

void KENLMBatch::EvaluateWhenApplied(const std::vector<Hypothesis*, MemPoolAllocator<Hypothesis*> > &hypos) const
{

}

void KENLMBatch::EvaluateWhenApplied(const Manager &mgr,
	const Hypothesis &hypo,
	const FFState &prevState,
	Scores &scores,
	FFState &state) const
{

}


void KENLMBatch::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, std::size_t &oovCount) const
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

lm::WordIndex KENLMBatch::TranslateID(const Word &word) const {
  std::size_t factor = word[m_factorType]->GetId();
  return (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
}

// Convert last words of hypothesis into vocab ids, returning an end pointer.
lm::WordIndex *KENLMBatch::LastIDs(const Hypothesis &hypo, lm::WordIndex *indices) const {
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
  return NULL;
}
