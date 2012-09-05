#include "SpanLengthFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "ChartHypothesis.h"
#include "DynSAInclude/types.h"

namespace Moses {
  
SpanLengthFeature::SpanLengthFeature(ScoreIndexManager &scoreIndexManager, const std::vector<float> &weight)
{
  scoreIndexManager.AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weight);
}
  
size_t SpanLengthFeature::GetNumScoreComponents() const
{
  return 1;
}
  
std::string SpanLengthFeature::GetScoreProducerDescription(unsigned) const
{
  return "SpanLength";
}

std::string SpanLengthFeature::GetScoreProducerWeightShortName(unsigned) const
{
  return "SL";
}

size_t SpanLengthFeature::GetNumInputScores() const {
  return 0;
}
  
const FFState* SpanLengthFeature::EmptyHypothesisState(const InputType &input) const
{
  return NULL; // not saving any kind of state so far
}

FFState* SpanLengthFeature::Evaluate(
  const Hypothesis &/*cur_hypo*/,
  const FFState */*prev_state*/,
  ScoreComponentCollection */*accumulator*/) const
{
  return NULL;
}
  
class SpanLengthFeatureState : public FFState
{
public:
  SpanLengthFeatureState(int terminalCount)
  : m_terminalCount(terminalCount)
  {
  }
  
  int GetTerminalCount() const {
    return m_terminalCount;
  }
  
  virtual int Compare(const FFState& other) const {
    int otherTerminalCount = (dynamic_cast<const SpanLengthFeatureState&>(other)).m_terminalCount;
    if (m_terminalCount < otherTerminalCount)
      return -1;
    if (m_terminalCount > otherTerminalCount)
      return 1;
    return 0;
  }
private:
  int m_terminalCount;
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
    const SpanLengthFeatureState* state = dynamic_cast<const SpanLengthFeatureState*>((*iter)->GetFFState(featureId));
    SpanInfo spanInfo = {
      curRange.GetNumWordsCovered(),
      state->GetTerminalCount(),
      curRange.GetStartPos()
    };
    result.push_back(spanInfo);
  }
  std::sort(result.begin(), result.end());
  return result;
}

FFState* SpanLengthFeature::EvaluateChart(
  const ChartHypothesis &chartHypothesis,
  int featureId,
  ScoreComponentCollection *accumulator) const
{
  std::vector<SpanInfo> spans = GetSpans(chartHypothesis, featureId);
  const TargetPhrase& targetPhrase = chartHypothesis.GetCurrTargetPhrase();
  int terminalCount = (int)chartHypothesis.GetCurrTargetPhrase().GetSize();
  for (size_t spanIndex = 0; spanIndex < spans.size(); ++spanIndex) {
    float prob = targetPhrase.GetScoreBySpanLengths(spanIndex, spans[spanIndex].SourceSpan, spans[spanIndex].TargetSpan);
    accumulator->PlusEquals(this, prob);
    terminalCount += spans[spanIndex].TargetSpan - 1;
  }
  return new SpanLengthFeatureState(terminalCount);
}
  
} // namespace moses