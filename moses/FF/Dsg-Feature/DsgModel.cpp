#include <fstream>
#include "DsgModel.h"
#include "dsgHyp.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;
using namespace lm::ngram;

namespace Moses
{

DesegModel::DesegModel(const std::string &line)
  :StatefulFeatureFunction(5, line )
{
  tFactor = 0;
  order=5;
  numFeatures = 5;
  optimistic = 1;
  ReadParameters();
}

DesegModel::~DesegModel()
{
  delete DSGM;
}

void DesegModel :: readLanguageModel(const char *lmFile)
{
  DSGM = ConstructDsgLM(m_lmPath.c_str());
  State startState = DSGM->NullContextState();
  desegT=new Desegmenter(m_desegPath,m_simple);// Desegmentation Table
}


void DesegModel::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  readLanguageModel(m_lmPath.c_str());
}



void DesegModel:: EvaluateInIsolation(const Phrase &source
                                      , const TargetPhrase &targetPhrase
                                      , ScoreComponentCollection &scoreBreakdown
                                      , ScoreComponentCollection &estimatedScores) const
{

  dsgHypothesis obj;
  vector <string> myTargetPhrase;
  vector<float> scores;
  vector<string> targ_phrase; //stores the segmented tokens in the target phrase
  const AlignmentInfo &align = targetPhrase.GetAlignTerm();

  for (int i = 0; i < targetPhrase.GetSize(); i++) {
    targ_phrase.push_back(targetPhrase.GetWord(i).GetFactor(tFactor)->GetString().as_string());
  }

  obj.setState(DSGM->NullContextState());
  obj.setPhrases(targ_phrase);
  obj.calculateDsgProbinIsol(*DSGM,*desegT,align);
  obj.populateScores(scores,numFeatures);
  estimatedScores.PlusEquals(this, scores);
}


FFState* DesegModel::EvaluateWhenApplied(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  const TargetPhrase &target = cur_hypo.GetCurrTargetPhrase();
  const Range &src_rng =cur_hypo.GetCurrSourceWordsRange();
  const AlignmentInfo &align = cur_hypo.GetCurrTargetPhrase().GetAlignTerm();
  size_t sourceOffset = src_rng.GetStartPos();

  dsgHypothesis obj;
  vector<float> scores;
  vector<string> targ_phrase; //stores the segmented tokens in the target phrase
  bool isCompleted;

  isCompleted=cur_hypo.IsSourceCompleted();
  for (int i = 0; i < cur_hypo.GetCurrTargetLength(); i++) {
    targ_phrase.push_back(target.GetWord(i).GetFactor(tFactor)->GetString().as_string());
  }

  obj.setState(prev_state);
  obj.setPhrases( targ_phrase );
  obj.calculateDsgProb(*DSGM,*desegT,isCompleted,align, sourceOffset, optimistic);
  obj.populateScores(scores,numFeatures);
  accumulator->PlusEquals(this, scores);
  return obj.saveState();

}

FFState* DesegModel::EvaluateWhenApplied(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  UTIL_THROW2("Chart decoding not support by UTIL_THROW2");
}

const FFState* DesegModel::EmptyHypothesisState(const InputType &input) const
{
  VERBOSE(3,"DesegModel::EmptyHypothesisState()" << endl);
  State startState = DSGM->BeginSentenceState();
  dsgState ss= dsgState(startState);
  return new dsgState(ss);
}

std::string DesegModel::GetScoreProducerWeightShortName(unsigned idx) const
{
  return "dsg";
}


void DesegModel::SetParameter(const std::string& key, const std::string& value)
{

  if (key == "path") {
    m_lmPath = value;
  } else if (key == "contiguity-features") {
    if(value == "no")
      numFeatures = 1;
    else
      numFeatures = 5;
  } else if (key == "output-factor") {
    tFactor = Scan<int>(value);
  } else if (key == "optimistic") {
    if (value == "n")
      optimistic = 0;
    else
      optimistic = 1;
  } else if (key == "deseg-path") {
    m_desegPath = Scan<int>(value);
  } else if (key == "deseg-scheme") {
    if(value == "s")
      m_simple = 1;
    else
      m_simple = 0;
  } else if (key == "order") {
    order = Scan<int>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

bool DesegModel::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[0];
  return ret;
}

} // namespace
