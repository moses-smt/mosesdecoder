#include "CrossingFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "ChartHypothesis.h"
#include "InputFileStream.h"
#include "UserMessage.h"
#include "DynSAInclude/types.h"

using namespace std;

namespace Moses {

CrossingFeatureData::CrossingFeatureData(const std::vector<std::string> &toks)
{
  const StaticData &staticData = StaticData::Instance();
  
  m_length  = Scan<int>(toks[0]);
  m_nonTerm.CreateFromString(Output, staticData.GetOutputFactorOrder(), toks[1], true);
  m_isCrossing = Scan<bool>(toks[2]);
  
}

CrossingFeatureData::CrossingFeatureData(size_t length, const Word &lhs, bool isCrossing)
:m_length(length)
,m_nonTerm(lhs)
,m_isCrossing(isCrossing)
{}

bool CrossingFeatureData::operator<(const CrossingFeatureData &compare) const
{
  if (m_length != compare.m_length)
    return m_length < compare.m_length;
  if (m_isCrossing != compare.m_isCrossing)
    return m_isCrossing;
  if (m_nonTerm != compare.m_nonTerm)
    return m_nonTerm < compare.m_nonTerm;
  return false;
}

////////////////////////////////////////////////////////////////
CrossingFeature::CrossingFeature(ScoreIndexManager &scoreIndexManager
                                 , const std::vector<float> &weights
                                 , const std::string &dataPath)
{
  CHECK(weights.size() == 1 || weights.size() == 2);
  scoreIndexManager.AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
  
  bool ret = LoadDataFile(dataPath);
  CHECK(ret);
}

bool CrossingFeature::LoadDataFile(const std::string &dataPath)
{
  InputFileStream inFile(dataPath);
  if (!inFile.good()) {
    UserMessage::Add(string("Couldn't read ") + dataPath);
    return false;
  }
  
  m_dataPath = dataPath;
  string line;
  size_t lineNum = 0;
  while(getline(inFile, line)) {
    vector<string> toks;
    Tokenize(toks, line);
    assert(toks.size() == 4);
    
    CrossingFeatureData data(toks);
    m_data[data] = Scan<float>(toks[3]);
    
    ++lineNum;
  }
  
  assert(lineNum == m_data.size());
  
  return true;
  
}

size_t CrossingFeature::GetNumScoreComponents() const
{
  return 1;
}
  
std::string CrossingFeature::GetScoreProducerDescription(unsigned id) const
{
  return "SpanLength";
}

std::string CrossingFeature::GetScoreProducerWeightShortName(unsigned id) const
{
  return "SL";
}

size_t CrossingFeature::GetNumInputScores() const {
  return 0;
}
  
const FFState* CrossingFeature::EmptyHypothesisState(const InputType &input) const
{
  return NULL; // not saving any kind of state so far
}

FFState* CrossingFeature::Evaluate(
  const Hypothesis &/*cur_hypo*/,
  const FFState */*prev_state*/,
  ScoreComponentCollection */*accumulator*/) const
{
  return NULL;
}
    
bool IsCrossingBool(const TargetPhrase& targetPhrase, size_t targetPos)
{
  const AlignmentInfo::NonTermIndexMap &nonTermIndex = targetPhrase.GetAlignmentInfo().GetNonTermIndexMap();
  
  int thisSourceInd = nonTermIndex[targetPos];
  
  for (size_t currPos = 0; currPos < targetPhrase.GetSize(); ++currPos)
  {
    const Word &word = targetPhrase.GetWord(currPos);
    
    if (currPos == targetPos)
    { // do nothing
    }
    else if (word.IsNonTerminal())
    {
      size_t currSourceInd = nonTermIndex[currPos];
      if  (currPos < targetPos && currSourceInd > thisSourceInd)
        return true;      
      else if  (currPos > targetPos && currSourceInd < thisSourceInd)
        return true;      
      
    }
    else
    { // terminal. TODO
      
    }
  }
  
  return false;  
}

float CrossingFeature::IsCrossing(const TargetPhrase& targetPhrase, const ChartHypothesis &chartHypothesis) const
{
  float score = 0;

  if (chartHypothesis.GetCurrSourceRange().GetStartPos() == 0)
    return score;
  
  if (chartHypothesis.GetCurrSourceRange() == WordsRange(1,5))
  {
    stringstream strme;
    strme << chartHypothesis.GetOutputPhrase();
    string str = strme.str();
    if (str == "they at least claim . ")
    {
      cerr << chartHypothesis.GetOutputPhrase() << endl
            << chartHypothesis.GetCurrTargetPhrase() << endl;
    }
  }
  
  const AlignmentInfo::NonTermIndexMap &nonTermIndex = targetPhrase.GetAlignmentInfo().GetNonTermIndexMap();

  map<CrossingFeatureData, float>::const_iterator iter;

  for (size_t targetPos = 0; targetPos < targetPhrase.GetSize(); ++targetPos)
  {
    const Word &word = targetPhrase.GetWord(targetPos);
    if (word.IsNonTerminal()) 
    {
      bool isCrossing = IsCrossingBool(targetPhrase, targetPos);

      int thisSourceInd = nonTermIndex[targetPos];
      const ChartHypothesis &child = *chartHypothesis.GetPrevHypo(thisSourceInd);
      size_t sourceSpan = child.GetCurrSourceRange().GetNumWordsCovered();
      
      CrossingFeatureData key(sourceSpan, word, isCrossing);
      iter = m_data.find(key);
      if (iter == m_data.end())
      { // novel entry. Hardcode
        score += log10(isCrossing ? 0.1 : 0.9);
      }
      else
      {
        score += log10(iter->second);
      }
    }
  }
  
  return score;
}
  
FFState* CrossingFeature::EvaluateChart(
  const ChartHypothesis &chartHypothesis,
  int featureId,
  ScoreComponentCollection *accumulator) const
{
  const TargetPhrase& targetPhrase = chartHypothesis.GetCurrTargetPhrase();
  
  // lookup score
  float score = IsCrossing(targetPhrase, chartHypothesis);  
  score = - FloorScore(score);
  
  accumulator->PlusEquals(this, score);
  return NULL;
}
  
} // namespace moses
