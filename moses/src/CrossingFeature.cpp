#include "CrossingFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "ChartHypothesis.h"
#include "DynSAInclude/types.h"
#include "InputFileStream.h"
#include "UserMessage.h"
#include "Util.h"

using namespace std;

namespace Moses {
  
CrossingFeatureData::CrossingFeatureData(const std::vector<std::string> &toks)
{
  m_length  = Scan<int>(toks[0]);
  m_nonTerm = toks[1];
  m_isCrossing = Scan<bool>(toks[2]);
  
}

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
  
class CrossingFeatureState : public FFState
{
public:
  CrossingFeatureState(unsigned terminalCount)
  : m_terminalCount(terminalCount)
  {
  }
  
  unsigned GetTerminalCount() const {
    return m_terminalCount;
  }
  
  virtual int Compare(const FFState& other) const {
    int otherTerminalCount = (dynamic_cast<const CrossingFeatureState&>(other)).m_terminalCount;
    if (m_terminalCount < otherTerminalCount)
      return -1;
    if (m_terminalCount > otherTerminalCount)
      return 1;
    return 0;
  }
private:
  unsigned m_terminalCount;
};
  
struct SpanInfo {
  unsigned SourceSpan, TargetSpan;
  unsigned SourceRangeStart;
};
  
static bool operator<(const SpanInfo& left, const SpanInfo& right)
{
  return left.SourceRangeStart < right.SourceRangeStart;
}
  
static std::vector<SpanInfo> GetSpans(const ChartHypothesis &chartHypothesis, int featureId)
{
  const std::vector<const ChartHypothesis*>& prevHypos = chartHypothesis.GetPrevHypos();
  std::vector<SpanInfo> result;
  result.reserve(prevHypos.size());
  iterate(prevHypos, iter) {
    WordsRange curRange = (*iter)->GetCurrSourceRange();
    SpanInfo spanInfo = {
      curRange.GetNumWordsCovered(),
      unsigned(-1),
      curRange.GetStartPos()
    };
    const CrossingFeatureState* state = dynamic_cast<const CrossingFeatureState*>((*iter)->GetFFState(featureId));
    if (state != NULL) {
      spanInfo.TargetSpan = state->GetTerminalCount();
    }
    result.push_back(spanInfo);
  }
  std::sort(result.begin(), result.end());
  return result;
}

FFState* CrossingFeature::EvaluateChart(
  const ChartHypothesis &chartHypothesis,
  int featureId,
  ScoreComponentCollection *accumulator) const
{
  std::vector<SpanInfo> spans = GetSpans(chartHypothesis, featureId);
  const TargetPhrase& targetPhrase = chartHypothesis.GetCurrTargetPhrase();
  const SpanLengthEstimatorCollection& spanLengthEstimators = targetPhrase.GetSpanLengthEstimators();
  std::vector<float> scores(GetNumScoreComponents());
  for (size_t spanIndex = 0; spanIndex < spans.size(); ++spanIndex) {
    scores[0] += spanLengthEstimators.GetScoreBySourceSpanLength(spanIndex, spans[spanIndex].SourceSpan);
  }

  unsigned terminalCount = (unsigned)chartHypothesis.GetCurrTargetPhrase().GetSize();
  for (size_t spanIndex = 0; spanIndex < spans.size(); ++spanIndex) {
    terminalCount += spans[spanIndex].TargetSpan - 1;
    scores[1] += spanLengthEstimators.GetScoreByTargetSpanLength(spanIndex, spans[spanIndex].TargetSpan);
  }
  accumulator->PlusEquals(this, scores);
  return new CrossingFeatureState(terminalCount);

}
  
} // namespace moses