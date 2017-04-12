/*
 * KENLM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <sstream>
#include <vector>
#include "KENLM.h"
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
#include "../SCFG/TargetPhraseImpl.h"
#include "../SCFG/Hypothesis.h"
#include "../SCFG/Manager.h"

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
class LanguageModelChartStateKenLM : public FFState
{
public:
  LanguageModelChartStateKenLM() {}

  const lm::ngram::ChartState &GetChartState() const {
    return m_state;
  }
  lm::ngram::ChartState &GetChartState() {
    return m_state;
  }

  size_t hash() const {
    size_t ret = hash_value(m_state);
    return ret;
  }
  virtual bool operator==(const FFState& o) const {
    const LanguageModelChartStateKenLM &other = static_cast<const LanguageModelChartStateKenLM &>(o);
    bool ret = m_state == other.m_state;
    return ret;
  }

  virtual std::string ToString() const {
    return "LanguageModelChartStateKenLM";
  }

private:
  lm::ngram::ChartState m_state;
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
template<class Model>
KENLM<Model>::KENLM(size_t startInd, const std::string &line,
                    const std::string &file, FactorType factorType,
                    util::LoadMethod load_method) :
  StatefulFeatureFunction(startInd, line), m_path(file), m_factorType(
    factorType), m_load_method(load_method)
{
  ReadParameters();
}

template<class Model>
KENLM<Model>::~KENLM()
{
  // TODO Auto-generated destructor stub
}

template<class Model>
void KENLM<Model>::Load(System &system)
{
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

template<class Model>
FFState* KENLM<Model>::BlankState(MemPool &pool, const System &sys) const
{
  FFState *ret;
  if (sys.isPb) {
    ret = new (pool.Allocate<KenLMState>()) KenLMState();
  } else {
    ret = new (pool.Allocate<LanguageModelChartStateKenLM>()) LanguageModelChartStateKenLM();
  }
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
template<class Model>
void KENLM<Model>::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                        const InputType &input, const Hypothesis &hypo) const
{
  KenLMState &stateCast = static_cast<KenLMState&>(state);
  stateCast.state = m_ngram->BeginSentenceState();
}

template<class Model>
void KENLM<Model>::EvaluateInIsolation(MemPool &pool, const System &system,
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

template<class Model>
void KENLM<Model>::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
                                       const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
                                       SCORE &estimatedScore) const
{
  // contains factors used by this LM
  float fullScore, nGramScore;
  size_t oovCount;

  CalcScore(targetPhrase, fullScore, nGramScore, oovCount);

  //float estimateScore = fullScore - nGramScore;

  // all LM scores are estimated
  float estimateScore = fullScore;
  nGramScore = 0;

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

template<class Model>
void KENLM<Model>::EvaluateWhenApplied(const ManagerBase &mgr,
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
  typename Model::State aux_state;
  typename Model::State *state0 = &stateCast.state, *state1 = &aux_state;

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

template<class Model>
void KENLM<Model>::CalcScore(const Phrase<Moses2::Word> &phrase, float &fullScore,
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

template<class Model>
void KENLM<Model>::CalcScore(const Phrase<SCFG::Word> &phrase, float &fullScore,
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
    const SCFG::Word &word = phrase[position];
    if (word.isNonTerminal) {
      fullScore += scorer.Finish();
      scorer.Reset();
    } else {
      lm::WordIndex index = TranslateID(word);
      scorer.Terminal(index);
      if (!index) ++oovCount;
    }
  }
  float before_boundary = fullScore + scorer.Finish();
  for (; position < phrase.GetSize(); ++position) {
    const SCFG::Word &word = phrase[position];
    if (word.isNonTerminal) {
      fullScore += scorer.Finish();
      scorer.Reset();
    } else {
      lm::WordIndex index = TranslateID(word);
      scorer.Terminal(index);
      if (!index) ++oovCount;
    }
  }
  fullScore += scorer.Finish();

  ngramScore = TransformLMScore(fullScore - before_boundary);
  fullScore = TransformLMScore(fullScore);
}

// Convert last words of hypothesis into vocab ids, returning an end pointer.
template<class Model>
lm::WordIndex *KENLM<Model>::LastIDs(const Hypothesis &hypo,
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

template<class Model>
void KENLM<Model>::EvaluateWhenApplied(const SCFG::Manager &mgr,
                                       const SCFG::Hypothesis &hypo, int featureID, Scores &scores,
                                       FFState &state) const
{
  LanguageModelChartStateKenLM &newState = static_cast<LanguageModelChartStateKenLM&>(state);
  lm::ngram::RuleScore<Model> ruleScore(*m_ngram, newState.GetChartState());
  const SCFG::TargetPhraseImpl &target = hypo.GetTargetPhrase();
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    target.GetAlignNonTerm().GetNonTermIndexMap();

  const size_t size = target.GetSize();
  size_t phrasePos = 0;
  // Special cases for first word.
  if (size) {
    const SCFG::Word &word = target[0];
    if (word[m_factorType] == m_bos) {
      // Begin of sentence
      ruleScore.BeginSentence();
      phrasePos++;
    } else if (word.isNonTerminal) {
      // Non-terminal is first so we can copy instead of rescoring.
      const SCFG::Hypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndexMap[phrasePos]);
      const lm::ngram::ChartState &prevState = static_cast<const LanguageModelChartStateKenLM*>(prevHypo->GetState(featureID))->GetChartState();
      ruleScore.BeginNonTerminal(prevState);
      phrasePos++;
    }
  }

  for (; phrasePos < size; phrasePos++) {
    const SCFG::Word &word = target[phrasePos];
    if (word.isNonTerminal) {
      const SCFG::Hypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndexMap[phrasePos]);
      const lm::ngram::ChartState &prevState = static_cast<const LanguageModelChartStateKenLM*>(prevHypo->GetState(featureID))->GetChartState();
      ruleScore.NonTerminal(prevState);
    } else {
      ruleScore.Terminal(TranslateID(word));
    }
  }

  float score = ruleScore.Finish();
  score = TransformLMScore(score);

  // take out score from loading. This needs reworking
  //score -= target.GetScores().GetScores(*this)[0];

  bool OOVFeatureEnabled = false;
  if (OOVFeatureEnabled) {
    std::vector<float> scoresVec(2);
    scoresVec[0] = score;
    scoresVec[1] = 0.0;
    scores.PlusEquals(mgr.system, *this, scoresVec);
  } else {
    scores.PlusEquals(mgr.system, *this, score);
  }
}

///////////////////////////////////////////////////////////////////////////

/* Instantiate LanguageModelKen here.  Tells the compiler to generate code
 * for the instantiations' non-inline member functions in this file.
 * Otherwise, depending on the compiler, those functions may not be present
 * at link time.
 */
template class KENLM<lm::ngram::ProbingModel> ;
template class KENLM<lm::ngram::RestProbingModel> ;
template class KENLM<lm::ngram::TrieModel> ;
template class KENLM<lm::ngram::ArrayTrieModel> ;
template class KENLM<lm::ngram::QuantTrieModel> ;
template class KENLM<lm::ngram::QuantArrayTrieModel> ;

FeatureFunction *ConstructKenLM(size_t startInd, const std::string &lineOrig)
{
  FactorType factorType = 0;
  string filePath;
  util::LoadMethod load_method = util::POPULATE_OR_READ;

  util::TokenIter<util::SingleCharacter, true> argument(lineOrig, ' ');
  ++argument; // KENLM

  util::StringStream line;
  line << "KENLM";

  for (; argument; ++argument) {
    const char *equals = std::find(argument->data(),
                                   argument->data() + argument->size(), '=');
    UTIL_THROW_IF2(equals == argument->data() + argument->size(),
                   "Expected = in KenLM argument " << *argument);
    StringPiece name(argument->data(), equals - argument->data());
    StringPiece value(equals + 1,
                      argument->data() + argument->size() - equals - 1);
    if (name == "factor") {
      factorType = boost::lexical_cast<FactorType>(value);
    } else if (name == "order") {
      // Ignored
    } else if (name == "path") {
      filePath.assign(value.data(), value.size());
    } else if (name == "lazyken") {
      // deprecated: use load instead.
      load_method =
        boost::lexical_cast<bool>(value) ?
        util::LAZY : util::POPULATE_OR_READ;
    } else if (name == "load") {
      if (value == "lazy") {
        load_method = util::LAZY;
      } else if (value == "populate_or_lazy") {
        load_method = util::POPULATE_OR_LAZY;
      } else if (value == "populate_or_read" || value == "populate") {
        load_method = util::POPULATE_OR_READ;
      } else if (value == "read") {
        load_method = util::READ;
      } else if (value == "parallel_read") {
        load_method = util::PARALLEL_READ;
      } else {
        UTIL_THROW2("Unknown KenLM load method " << value);
      }
    } else {
      // pass to base class to interpret
      line << " " << name << "=" << value;
    }
  }

  return ConstructKenLM(startInd, line.str(), filePath, factorType, load_method);
}

FeatureFunction *ConstructKenLM(size_t startInd, const std::string &line,
                                const std::string &file, FactorType factorType,
                                util::LoadMethod load_method)
{
  lm::ngram::ModelType model_type;
  if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
    switch (model_type) {
    case lm::ngram::PROBING:
      return new KENLM<lm::ngram::ProbingModel>(startInd, line, file,
             factorType, load_method);
    case lm::ngram::REST_PROBING:
      return new KENLM<lm::ngram::RestProbingModel>(startInd, line, file,
             factorType, load_method);
    case lm::ngram::TRIE:
      return new KENLM<lm::ngram::TrieModel>(startInd, line, file, factorType,
                                             load_method);
    case lm::ngram::QUANT_TRIE:
      return new KENLM<lm::ngram::QuantTrieModel>(startInd, line, file,
             factorType, load_method);
    case lm::ngram::ARRAY_TRIE:
      return new KENLM<lm::ngram::ArrayTrieModel>(startInd, line, file,
             factorType, load_method);
    case lm::ngram::QUANT_ARRAY_TRIE:
      return new KENLM<lm::ngram::QuantArrayTrieModel>(startInd, line, file,
             factorType, load_method);
    default:
      UTIL_THROW2("Unrecognized kenlm model type " << model_type)
      ;
    }
  } else {
    return new KENLM<lm::ngram::ProbingModel>(startInd, line, file, factorType,
           load_method);
  }
}

}

