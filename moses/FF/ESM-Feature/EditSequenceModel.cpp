#include <fstream>
#include <set>
#include <boost/foreach.hpp>

#include "EditSequenceModel.h"
#include "moses/Util.h"
#include "util/exception.hh"

#include "UtilESM.h"

using namespace std;
using namespace lm::ngram;

namespace Moses
{

esmState::esmState(const State& val)
{
  lmState = val;
}

void esmState::setLMState(const State& val) {
  lmState = val;
}

int esmState::Compare(const FFState& otherBase) const
{
  const esmState &other = static_cast<const esmState&>(otherBase);
  if (lmState.length < other.lmState.length) return -1;
  if (lmState.length > other.lmState.length) return 1;
  return 0;
}

std::string esmState :: getName() const
{
  return "done";
}                 

EditSequenceModel::EditSequenceModel(const std::string &line)
  :StatefulFeatureFunction(1, line )
{
  m_sFactor = 0;
  m_tFactor = 0;
  numFeatures = 1;
  ReadParameters();
}

EditSequenceModel::~EditSequenceModel() {
  delete ESM;
}

void EditSequenceModel :: readLanguageModel(const char *lmFile)
{
  string unkOp = "1_";
  ESM = ConstructESMLM(m_lmPath);

  State startState = ESM->NullContextState();
  State endState;
  unkOpProb = ESM->Score(startState, unkOp, endState);
}

void EditSequenceModel::Load()
{
  readLanguageModel(m_lmPath.c_str());
}

void EditSequenceModel:: EvaluateInIsolation(const Phrase &source
                                , const TargetPhrase &targetPhrase
                                , ScoreComponentCollection &scoreBreakdown
                                , ScoreComponentCollection &estimatedFutureScore) const
{
  std::vector<std::string> mySourcePhrase;
  std::vector<std::string> myTargetPhrase;
  std::vector<size_t> alignments;

  for (size_t i = 0; i < source.GetSize(); i++)
    mySourcePhrase.push_back(source.GetWord(i).GetFactor(m_sFactor)->GetString().as_string());

  for (size_t i = 0; i < targetPhrase.GetSize(); i++) {
    if(targetPhrase.GetWord(i).IsOOV() && m_sFactor == 0 && m_tFactor == 0)
      myTargetPhrase.push_back("1_");
    else
      myTargetPhrase.push_back(targetPhrase.GetWord(i).GetFactor(m_tFactor)->GetString().as_string());
  }
  
  const AlignmentInfo &alignment = targetPhrase.GetAlignTerm();
  std::vector<std::string> edits;
  calculateEdits(edits, mySourcePhrase, myTargetPhrase, alignment);   

  esmState start_state(ESM->NullContextState());
  esmState curr_state(ESM->NullContextState());
  float opProb = calculateScore(edits, &start_state, &curr_state, false);
    
  std::vector<float> scores;
  scores.push_back(opProb);
  estimatedFutureScore.PlusEquals(this, scores);
}


FFState* EditSequenceModel::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  std::vector<std::string> mySourcePhrase;
  std::vector<std::string> myTargetPhrase;
  std::vector<size_t> alignments;

  const Manager &manager = cur_hypo.GetManager();
  const InputType &source = manager.GetSource();
  const WordsRange& sourceRange = cur_hypo.GetCurrSourceWordsRange();
  size_t startIndex  = sourceRange.GetStartPos();
  size_t endIndex = sourceRange.GetEndPos();
  for (size_t i = startIndex; i <= endIndex; i++)
    mySourcePhrase.push_back(source.GetWord(i).GetFactor(m_sFactor)->GetString().as_string());

  const TargetPhrase &target = cur_hypo.GetCurrTargetPhrase();
  for (size_t i = 0; i < target.GetSize(); i++) {
    if(target.GetWord(i).IsOOV() && m_sFactor == 0 && m_tFactor == 0)
      myTargetPhrase.push_back("1_");
    else
      myTargetPhrase.push_back(target.GetWord(i).GetFactor(m_tFactor)->GetString().as_string());
  }
  const AlignmentInfo &alignment = target.GetAlignTerm();
  std::vector<std::string> edits;
  calculateEdits(edits, mySourcePhrase, myTargetPhrase, alignment);

  FFState* curr_state = new esmState(ESM->NullContextState());
  float opProb = calculateScore(edits, prev_state, curr_state, cur_hypo.IsSourceCompleted());

  std::vector<float> scores;
  scores.push_back(opProb);
  accumulator->PlusEquals(this, scores);

  return curr_state;
}

float EditSequenceModel::calculateScore(
             const std::vector<std::string>& sequence,
             const FFState* prev_state,
             FFState* curr_state,
             bool completed) const {

  float opProb = 0;
  lm::ngram::State currLMState = static_cast<const esmState*>(prev_state)->getLMState();
  lm::ngram::State temp;

  for(size_t i = 0; i < sequence.size(); i++) {
    temp = currLMState;
    opProb += ESM->Score(temp, sequence[i], currLMState);
  }

  if(completed) {
    temp = currLMState;
    opProb += ESM->SentenceEndScore(temp, currLMState);
  }

  static_cast<esmState*>(curr_state)->setLMState(currLMState);
  return opProb;
}

FFState* EditSequenceModel::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
    UTIL_THROW2("Chart decoding not support by UTIL_THROW2");
}

const FFState* EditSequenceModel::EmptyHypothesisState(const InputType &input) const
{
  VERBOSE(3,"EditSequenceModel::EmptyHypothesisState()" << endl);

  State startState = ESM->BeginSentenceState();
  return new esmState(startState);
}

std::string EditSequenceModel::GetScoreProducerWeightShortName(unsigned idx) const
{
  return "esm";
}

void EditSequenceModel::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_lmPath = value;
  } else if (key == "support-features") {
    if(value == "no") {
      numFeatures = 1;
      m_numScoreComponents = 1;
    }
    else {
      numFeatures = 1;
    }
  } else if (key == "input-factor") {
    m_sFactor = Scan<int>(value);
  } else if (key == "output-factor") {
    m_tFactor = Scan<int>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

bool EditSequenceModel::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[0];
  return ret;
}

} // namespace
