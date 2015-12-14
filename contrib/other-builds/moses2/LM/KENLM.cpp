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
#include "lm/model.hh"
#include "lm/lookup.hh"
#include "../legacy/FactorCollection.h"

using namespace std;

struct KenLMState : public FFState {
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

};

/////////////////////////////////////////////////////////////////

// note: prefetching is only supported with ProbingModel

/**
 * Executes an LM query from the decoder by parts. Query here means scoring an added phrase's
 * boundary (scoring the phrase without internal n-grams). This means potentially several words.
 *
 * Used exclusively by PrefetchQueue.
 *
 * This is where it gets slightly gory. I am inspired by various unstructured code that
 * interfaces to things I don't dare touch.
 */
struct PrefetchQueueEntry {
  Hypothesis *hypo;
  lm::ngram::detail::PrefetchLookup<lm::ngram::ProbingModel::SearchType, lm::ngram::ProbingModel::VocabularyType> lookup;

  float score;

  size_t position;
  size_t adjust_end;
  bool finished;

  size_t m_statefulInd; ///< stateful FF index of our underlying FF
  const KENLM *m_kenlm;

  //PrefetchQueueEntry(): finished(true), m_kenlm(*static_cast<KENLM *>(NULL)) {}
  PrefetchQueueEntry(): finished(true), m_kenlm(NULL) {}

  /**
   * One-time initialization that should be the constructor, but can't be, due to us being an array element.
   */
  void Construct(lm::ngram::ProbingModel &model, const KENLM &kenlm) {
    lookup.Construct(&model.GetVocab(), &model.GetSearch(), model.Order());
    m_statefulInd = kenlm.GetStatefulInd();
    m_kenlm = &kenlm;
  }

  /**
   * Prepare for prefetched scoring of a new Hypothesis.
   */
  void Init(Hypothesis *hypo) {
    this->hypo = hypo;

    FFState *prevState = hypo->GetPrevHypo()->GetState(m_statefulInd);
    const lm::ngram::State &in_state = static_cast<const KenLMState *>(prevState)->state;

    if (!hypo->GetTargetPhrase().GetSize()) {
      // not the nicest way of setting state...
      *(hypo->GetState(m_statefulInd)) = *prevState;
      finished = true;
      return;
    }

    // IMHO, adjust_end should be on the Hypothesis since we keep using it.
    // [begin, end) in STL-like fashion.
    const std::size_t begin = hypo->GetCurrTargetWordsRange().GetStartPos();
    const std::size_t end = hypo->GetCurrTargetWordsRange().GetEndPos() + 1;
    // we only score the beginning of the phrase (which depends on the choice of previous Hypothesis)
    adjust_end = std::min(end, begin + lookup.Order() - 1);

    // prepare lookup of first word
    position = begin;
    finished = false;
    lookup.Init(in_state, m_kenlm->TranslateID(hypo->GetWord(position++)));
  }

  bool Finished() {
    return finished;
  }

  /** Keep calling this until it returns false. */
  bool RunState(const Manager &mgr) {
    if(finished)
      return false;

    if(lookup.RunState())
      // more calls for the same word (different n-gram orders)
      return true;

    // have result of lookup for word
    score += lookup.FullScore().prob;

    if(position == adjust_end) {
      // not the nicest way of setting state...
      static_cast<KenLMState *>(hypo->GetState(m_statefulInd))->state = lookup.GetOutState();
      // set score, and potentially state as well.
      m_kenlm->FinalizeEvaluateWhenApplied(score, mgr, *hypo, hypo->GetScores(), *hypo->GetState(m_statefulInd));
      finished = true;
      return false;
    }

    // init for next word
    lookup.Init(lookup.GetOutState(), m_kenlm->TranslateID(hypo->GetWord(position++)));
    return true;
  }
};

/**
 * A circular buffer for scoring Hypotheses in several steps, which involve prefetching.
 */
template<unsigned PrefetchSize>
class PrefetchQueue {
public:
  PrefetchQueue(lm::ngram::ProbingModel &model, const KENLM &kenlm): m_cur(0), m_model(model) {
    // I avoid constructor and operator= here since this is going to happen very often.
    for(PrefetchQueueEntry *i = m_entries; i != m_entries + PrefetchSize; i++)
      i->Construct(model, kenlm);
  }

  void Add(Hypothesis *hypo) {
    // can only add entries when there is space
    assert(Cur().Finished());

    Cur().Init(hypo);
    Next();
  }

  /** Keep calling this until it returns false, then you can add another entry. */
  bool RunState(const Manager &mgr) {
    // empty slot?
    if(Cur().Finished())
      return false;
    // run step. Just finished?
    if(!Cur().RunState(mgr))
      return false;
    // still not done. Run the subsequent entry next time.
    Next();
    return true;
  }

  /** Finish processing. */
  void Drain(const Manager &mgr) {
    bool anyBusy = true;
    while(anyBusy) {
      anyBusy = false;
      for(PrefetchQueueEntry *i = m_entries; i != m_entries + PrefetchSize; i++)
        anyBusy = anyBusy || i->RunState(mgr);
    }
  }

private:
  PrefetchQueueEntry& Cur() {
    return m_entries[m_cur];
  }

  void Next() {
    m_cur++;
    m_cur = m_cur % PrefetchSize;
  }

  PrefetchQueueEntry m_entries[PrefetchSize];
  std::size_t m_cur;

  const lm::ngram::ProbingModel &m_model;
};

void KENLM::EvaluateWhenAppliedBatched(Hypothesis **begin, Hypothesis **end, const Manager &mgr) const
{
  Hypothesis **i;
  for(i = begin; i != end; ++i) {
    // work queue to make space
    while(m_prefetchQueue->RunState(mgr));
    m_prefetchQueue->Add(*i);
  }
  // finish work
  m_prefetchQueue->Drain(mgr);
}


/////////////////////////////////////////////////////////////////
class MappingBuilder : public lm::EnumerateVocab
{
public:
  MappingBuilder(FactorCollection &factorCollection, System &system, size_t vocabInd)
    : m_factorCollection(factorCollection)
	, m_system(system)
	, m_vocabInd(vocabInd)
  {}

  void Add(lm::WordIndex index, const StringPiece &str) {
	const Factor *factor = m_factorCollection.AddFactor(str, m_system);
	//cerr << "m_vocabInd=" << m_vocabInd << " ffData=" << factor->ffData.size() << endl;

	factor->ffData[m_vocabInd] = (void*) index;
  }

private:
  FactorCollection &m_factorCollection;
  System &m_system;
  size_t m_vocabInd;
};

/////////////////////////////////////////////////////////////////
KENLM::KENLM(size_t startInd, const std::string &line)
:BatchedFeatureFunction(startInd, line)
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

  m_bos = fc.AddFactor("<s>", system, false);
  m_eos = fc.AddFactor("</s>", system, false);

  lm::ngram::Config config;
  config.messages = NULL;

  FactorCollection &collection = system.GetVocab();
  MappingBuilder builder(collection, system, m_vocabInd);
  config.enumerate_vocab = &builder;
  config.load_method = m_lazy ? util::LAZY : util::POPULATE_OR_READ;

  m_ngram.reset(new Model(m_path.c_str(), config));

  // a tight, looped queue of about 4 hypotheses for prefetching
  m_prefetchQueue.reset(new PrefetchQueue<4>(*m_ngram.get(), *this));
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

FFState* KENLM::BlankState(const Manager &mgr, const PhraseImpl &input) const
{
  MemPool &pool = mgr.GetPool();
  KenLMState *ret = new (pool.Allocate<KenLMState>()) KenLMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void KENLM::EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const
{
  KenLMState &stateCast = static_cast<KenLMState&>(state);
  stateCast.state = m_ngram->BeginSentenceState();
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

// handle end of sentence, long phrases, converting and writing score
void KENLM::FinalizeEvaluateWhenApplied(float ngramScore,
                                        const Manager &mgr,
                                        const Hypothesis &hypo,
                                        Scores &scores,
                                        FFState &state) const
{
  float score = ngramScore;
  KenLMState &stateCast = static_cast<KenLMState&>(state);
  const System &system = mgr.system;

  // [begin, end) in STL-like fashion.
  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_ngram->Order() - 1);

  if (hypo.GetBitmap().IsComplete()) {
    // Score end of sentence.
    std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
    const lm::WordIndex *last = LastIDs(hypo, &indices.front());
    score += m_ngram->FullScoreForgotState(&indices.front(), last, m_ngram->GetVocabulary().EndSentence(), stateCast.state).prob;
  } else if (adjust_end < end) {
    // Get state after adding a long phrase.
    std::vector<lm::WordIndex> indices(m_ngram->Order() - 1);
    const lm::WordIndex *last = LastIDs(hypo, &indices.front());
    m_ngram->GetState(&indices.front(), last, stateCast.state);
  }
  // for short phrases, output state was already set before.

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


void KENLM::EvaluateWhenApplied(const Manager &mgr,
  const Hypothesis &hypo,
  const FFState &prevState,
  Scores &scores,
  FFState &state) const
{
  assert(0 && "This method should not be called on a BatchedFeatureFunction.");

  KenLMState &stateCast = static_cast<KenLMState&>(state);

  const System &system = mgr.system;

  const lm::ngram::State &in_state = static_cast<const KenLMState&>(prevState).state;

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
  typename Model::State *state0 = &stateCast.state, *state1 = &aux_state;

  float score = m_ngram->Score(in_state, TranslateID(hypo.GetWord(position)), *state0);
  ++position;
  for (; position < adjust_end; ++position) {
    score += m_ngram->Score(*state0, TranslateID(hypo.GetWord(position)), *state1);
    std::swap(state0, state1);
  }
  if (state0 != &stateCast.state) {
    // Short enough phrase that we can just reuse the state.
    stateCast.state = *state0;
  }

  // handle end of sentence, long phrases, converting and writing score
  FinalizeEvaluateWhenApplied(score, mgr, hypo, scores, state);
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

lm::WordIndex KENLM::TranslateID(const Word &word) const {
  const Factor *factor = word[m_factorType];
  lm::WordIndex ret = (lm::WordIndex)(size_t) factor->ffData[m_vocabInd];
  return ret;
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
  return NULL;
}
