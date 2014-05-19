
#include <boost/functional/hash.hpp>
#include "moses/FF/FFState.h"
#include "DALMWrapper.h"
#include "logger.h"
#include "dalm.h"
#include "vocabulary.h"
#include "moses/FactorTypeSet.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "util/exception.hh"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"

using namespace std;

/////////////////////////
void read_ini(const char *inifile, string &model, string &words, string &wordstxt)
{
  ifstream ifs(inifile);
  string line;

  getline(ifs, line);
  while(ifs) {
    unsigned int pos = line.find("=");
    string key = line.substr(0, pos);
    string value = line.substr(pos+1, line.size()-pos);
    if(key=="MODEL") {
      model = value;
    } else if(key=="WORDS") {
      words = value;
    } else if(key=="WORDSTXT") {
      wordstxt = value;
    }
    getline(ifs, line);
  }
}

/////////////////////////

namespace Moses
{

class DALMState : public FFState
{
private:
  DALM::State *state;

public:
  DALMState(unsigned short order) {
    state = new DALM::State(order);
  }

  DALMState(const DALMState &from) {
    state = new DALM::State(*from.state);
  }

  virtual ~DALMState() {
    delete state;
  }

  void reset(const DALMState &from) {
    delete state;
    state = new DALM::State(*from.state);
  }

  void reset(DALM::State *s) {
    delete state;
    state = s;
  }

  virtual int Compare(const FFState& other) const {
    const DALMState &o = static_cast<const DALMState &>(other);
    if(state->get_count() < o.state->get_count()) return -1;
    else if(state->get_count() > o.state->get_count()) return 1;
    else return state->compare(o.state);
  }

  DALM::State *get_state() const {
    return state;
  }

  void refresh() {
    state->refresh();
  }
};

class DALMChartState : public FFState
{
private:
  const ChartHypothesis &hypo;
  DALM::Fragment *prefixFragments;
  unsigned short prefixLength;
  float prefixScore;
  DALMState *rightContext;
  bool isLarge;

public:
  DALMChartState(
    const ChartHypothesis &hypo,
    DALM::Fragment *prefixFragments,
    unsigned short prefixLength,
    float prefixScore,
    DALMState *rightContext,
    bool isLarge)
    : hypo(hypo),
      prefixFragments(prefixFragments),
      prefixLength(prefixLength),
      prefixScore(prefixScore),
      rightContext(rightContext),
      isLarge(isLarge)
  {}

  virtual ~DALMChartState() {
    delete [] prefixFragments;
    delete rightContext;
  }

  unsigned short GetPrefixLength() const {
    return prefixLength;
  }

  const DALM::Fragment *GetPrefixFragments() const {
    return prefixFragments;
  }

  float GetPrefixScore() const {
    return prefixScore;
  }

  const DALMState *GetRightContext() const {
    return rightContext;
  }

  bool LargeEnough() const {
    return isLarge;
  }

  virtual int Compare(const FFState& other) const {
    const DALMChartState &o = static_cast<const DALMChartState &>(other);
    // prefix
    if (hypo.GetCurrSourceRange().GetStartPos() > 0) { // not for "<s> ..."
      if (prefixLength != o.prefixLength) {
        return (prefixLength < o.prefixLength)?-1:1;
      } else {
        if(prefixLength > 0) {
          DALM::Fragment &f = prefixFragments[prefixLength-1];
          DALM::Fragment &of = o.prefixFragments[prefixLength-1];
          int ret = DALM::compare_fragments(f, of);
          if(ret != 0) return ret;
        }
      }
    }

    // suffix
    size_t inputSize = hypo.GetManager().GetSource().GetSize();
    if (hypo.GetCurrSourceRange().GetEndPos() < inputSize - 1) { // not for "... </s>"
      int ret = o.rightContext->Compare(*rightContext);
      if (ret != 0) return ret;
    }
    return 0;
  }
};

LanguageModelDALM::LanguageModelDALM(const std::string &line)
  :LanguageModel(line)
{
  ReadParameters();

  if (m_factorType == NOT_FOUND) {
    m_factorType = 0;
  }
}

LanguageModelDALM::~LanguageModelDALM()
{
  delete m_logger;
  delete m_vocab;
  delete m_lm;
}

void LanguageModelDALM::Load()
{
  /////////////////////
  // READING INIFILE //
  /////////////////////
  string inifile= m_filePath + "/dalm.ini";

  UTIL_THROW_IF(model.empty() || words.empty() || wordstxt.empty(),
                util::FileOpenException,
                "Failed to read DALM ini file " << m_filePath << ". Probably doesn't exist");

  model = m_filePath + "/" + model;
  words = m_filePath + "/" + words;
  wordstxt = m_filePath + "/" + wordstxt;

  // Preparing a logger object.
  m_logger = new DALM::Logger(stderr);
  m_logger->setLevel(DALM::LOGGER_INFO);

  // Load the vocabulary file.
  m_vocab = new DALM::Vocabulary(words, *m_logger);

  // Load the language model.
  m_lm = new DALM::LM(model, *m_vocab, *m_logger);

  wid_start = m_vocab->lookup(BOS_);
  wid_end = m_vocab->lookup(EOS_);

  // vocab mapping
  CreateVocabMapping(wordstxt);

  FactorCollection &collection = FactorCollection::Instance();
  m_beginSentenceFactor = collection.AddFactor(BOS_);
}

const FFState *LanguageModelDALM::EmptyHypothesisState(const InputType &/*input*/) const
{
  DALMState *s = new DALMState(m_nGramOrder);
  m_lm->init_state(*s->get_state());
  return s;
}

void LanguageModelDALM::CalcScore(const Phrase &phrase, float &fullScore, float &ngramScore, size_t &oovCount) const
{
  fullScore  = 0;
  ngramScore = 0;

  oovCount = 0;

  size_t phraseSize = phrase.GetSize();
  if (!phraseSize) return;
  size_t currPos = 0;
  size_t hist_count = 0;
  DALMState *dalm_state = new DALMState(m_nGramOrder);
  DALM::State *state = dalm_state->get_state();

  if(phrase.GetWord(0).GetFactor(m_factorType) == m_beginSentenceFactor) {
    m_lm->init_state(*state);
    currPos++;
    hist_count++;
  }

  while (currPos < phraseSize) {
    const Word &word = phrase.GetWord(currPos);
    hist_count++;

    if (word.IsNonTerminal()) {
      state->refresh();
      hist_count = 0;
    } else {
      DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
      float score = m_lm->query(wid, *state);
      fullScore += score;
      if (hist_count >= m_nGramOrder) ngramScore += score;
      if (wid==m_vocab->unk()) ++oovCount;
    }

    currPos++;
  }

  fullScore = TransformLMScore(fullScore);
  ngramScore = TransformLMScore(ngramScore);
  delete dalm_state;
}

FFState *LanguageModelDALM::Evaluate(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const
{
  // In this function, we only compute the LM scores of n-grams that overlap a
  // phrase boundary. Phrase-internal scores are taken directly from the
  // translation option.

  const DALMState *dalm_ps = static_cast<const DALMState *>(ps);

  // Empty phrase added? nothing to be done
  if (hypo.GetCurrTargetLength() == 0) {
    return dalm_ps ? new DALMState(*dalm_ps) : NULL;
  }

  const std::size_t begin = hypo.GetCurrTargetWordsRange().GetStartPos();
  //[begin, end) in STL-like fashion.
  const std::size_t end = hypo.GetCurrTargetWordsRange().GetEndPos() + 1;
  const std::size_t adjust_end = std::min(end, begin + m_nGramOrder - 1);

  DALMState *dalm_state = new DALMState(*dalm_ps);
  DALM::State *state = dalm_state->get_state();

  float score = 0.0;
  for(std::size_t position=begin; position < adjust_end; position++) {
    score += m_lm->query(GetVocabId(hypo.GetWord(position).GetFactor(m_factorType)), *state);
  }

  if (hypo.IsSourceCompleted()) {
    // Score end of sentence.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), *state);

    score += m_lm->query(wid_end, *state);
  } else if (adjust_end < end) {
    // Get state after adding a long phrase.
    std::vector<DALM::VocabId> indices(m_nGramOrder-1);
    const DALM::VocabId *last = LastIDs(hypo, &indices.front());
    m_lm->set_state(&indices.front(), (last-&indices.front()), *state);
  }

  score = TransformLMScore(score);
  if (OOVFeatureEnabled()) {
    std::vector<float> scores(2);
    scores[0] = score;
    scores[1] = 0.0;
    out->PlusEquals(this, scores);
  } else {
    out->PlusEquals(this, score);
  }

  return dalm_state;
}

FFState *LanguageModelDALM::EvaluateChart(const ChartHypothesis& hypo, int featureID, ScoreComponentCollection *out) const
{
  // initialize language model context state
  DALMState *dalm_state = new DALMState(m_nGramOrder);
  DALM::State *state = dalm_state->get_state();

  size_t contextSize = m_nGramOrder-1;
  DALM::Fragment *prefixFragments = new DALM::Fragment[contextSize];
  unsigned short prefixLength = 0;
  bool isLarge = false;

  // initial language model scores
  float prefixScore = 0.0;    // not yet final for initial words (lack context)
  float hypoScore = 0.0;      // total hypothesis score.

  const TargetPhrase &targetPhrase = hypo.GetCurrTargetPhrase();
  size_t hypoSize = targetPhrase.GetSize();

  // get index map for underlying hypotheses
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    targetPhrase.GetAlignNonTerm().GetNonTermIndexMap();

  size_t phrasePos = 0;

  // begginig of sentence.
  if(hypoSize > 0) {
    const Word &word = targetPhrase.GetWord(0);
    if(!word.IsNonTerminal()) {
      DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
      if(word.GetFactor(m_factorType) == m_beginSentenceFactor) {
        m_lm->init_state(*state);
        // state is finalized.
        isLarge = true;
      } else {
        if(isLarge) {
          float score = m_lm->query(wid, *state);
          hypoScore += score;
        } else {
          float score = m_lm->query(wid, *state, prefixFragments[prefixLength]);

          prefixScore += score;
          hypoScore += score;
          prefixLength++;
          if(prefixLength >= contextSize) isLarge = true;
        }
      }
    } else {
      // special case: rule starts with non-terminal -> copy everything
      size_t nonTermIndex = nonTermIndexMap[0];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);

      const DALMChartState* prevState =
        static_cast<const DALMChartState*>(prevHypo->GetFFState(featureID));

      // get prefixScore and hypoScore
      prefixScore = prevState->GetPrefixScore();
      hypoScore = UntransformLMScore(prevHypo->GetScoreBreakdown().GetScoresForProducer(this)[0]);

      // get language model state
      dalm_state->reset(*prevState->GetRightContext());
      state = dalm_state->get_state();

      prefixLength = prevState->GetPrefixLength();
      const DALM::Fragment *prevPrefixFragments = prevState->GetPrefixFragments();
      std::memcpy(prefixFragments, prevPrefixFragments, sizeof(DALM::Fragment)*prefixLength);
      isLarge = prevState->LargeEnough();
    }
    phrasePos++;
  }

  // loop over rule
  for (; phrasePos < hypoSize; phrasePos++) {
    // consult rule for either word or non-terminal
    const Word &word = targetPhrase.GetWord(phrasePos);

    // regular word
    if (!word.IsNonTerminal()) {
      DALM::VocabId wid = GetVocabId(word.GetFactor(m_factorType));
      if (isLarge) {
        hypoScore += m_lm->query(wid, *state);
      } else {
        float score = m_lm->query(wid, *state, prefixFragments[prefixLength]);
        prefixScore += score;
        hypoScore += score;
        prefixLength++;
        if(prefixLength >= contextSize) isLarge = true;
      }
    }

    // non-terminal, add phrase from underlying hypothesis
    // internal non-terminal
    else {
      // look up underlying hypothesis
      size_t nonTermIndex = nonTermIndexMap[phrasePos];
      const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);

      const DALMChartState* prevState =
        static_cast<const DALMChartState*>(prevHypo->GetFFState(featureID));

      size_t prevPrefixLength = prevState->GetPrefixLength();
      const DALM::Fragment *prevPrefixFragments = prevState->GetPrefixFragments();
      DALM::Gap gap(*state);
      // score its prefix
      for(size_t prefixPos = 0; prefixPos < prevPrefixLength; prefixPos++) {
        const DALM::Fragment &f = prevPrefixFragments[prefixPos];

        if (isLarge) {
          hypoScore += m_lm->query(f, *state, gap);
        } else {
          float score = m_lm->query(f, *state, gap, prefixFragments[prefixLength]);
          prefixScore += score;
          hypoScore += score;
          prefixLength++;
          if(prefixLength >= contextSize) isLarge = true;
        }
        gap.succ();
      }

      // check if we are dealing with a large sub-phrase
      if (prevState->LargeEnough()) {
        // add its language model score
        hypoScore += UntransformLMScore(prevHypo->GetScoreBreakdown().GetScoresForProducer(this)[0]);
        hypoScore -= prevState->GetPrefixScore(); // remove overwrapped score.
        // copy language model state
        dalm_state->reset(*prevState->GetRightContext());
        state = dalm_state->get_state();
      } else {
        DALM::State *state_new = new DALM::State(*prevState->GetRightContext()->get_state());
        m_lm->set_state(*state_new, *state, gap);
        dalm_state->reset(state_new);
        state = dalm_state->get_state();
      }
    }
  }

  // assign combined score to score breakdown
  out->Assign(this, TransformLMScore(hypoScore));

  return new DALMChartState(hypo, prefixFragments, prefixLength, prefixScore, dalm_state, isLarge);
}

bool LanguageModelDALM::IsUseable(const FactorMask &mask) const
{
  return mask[m_factorType];
}

void LanguageModelDALM::CreateVocabMapping(const std::string &wordstxt)
{
  InputFileStream vocabStrm(wordstxt);

  std::vector< std::pair<std::size_t, DALM::VocabId> > vlist;
  string line;
  std::size_t max_fid = 0;
  while(getline(vocabStrm, line)) {
    const Factor *factor = FactorCollection::Instance().AddFactor(line);
    std::size_t fid = factor->GetId();
    DALM::VocabId wid = m_vocab->lookup(line.c_str());

    vlist.push_back(std::pair<std::size_t, DALM::VocabId>(fid, wid));
    if(max_fid < fid) max_fid = fid;
  }

  for(std::size_t i = 0; i < m_vocabMap.size(); i++) {
    m_vocabMap[i] = m_vocab->unk();
  }

  m_vocabMap.resize(max_fid+1, m_vocab->unk());
  std::vector< std::pair<std::size_t, DALM::VocabId> >::iterator it = vlist.begin();
  while(it != vlist.end()) {
    std::pair<std::size_t, DALM::VocabId> &entry = *it;
    m_vocabMap[entry.first] = entry.second;

    ++it;
  }
}

DALM::VocabId LanguageModelDALM::GetVocabId(const Factor *factor) const
{
  std::size_t fid = factor->GetId();
  return (m_vocabMap.size() > fid)? m_vocabMap[fid] : m_vocab->unk();
}

void LanguageModelDALM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "order") {
    m_nGramOrder = Scan<size_t>(value);
  } else if (key == "path") {
    m_filePath = value;
  } else {
    LanguageModel::SetParameter(key, value);
  }
}

}
