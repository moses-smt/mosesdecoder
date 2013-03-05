#include "SpanLengthFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "ChartHypothesis.h"
#include "DynSAInclude/types.h"

namespace Moses {
  
SpanLengthFeature::SpanLengthFeature(ScoreIndexManager &scoreIndexManager, const std::vector<float> &weights)
  : m_withTargetLength(weights.size() > 1)
{
  CHECK(weights.size() == 1 || weights.size() == 2);
  scoreIndexManager.AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}
  
size_t SpanLengthFeature::GetNumScoreComponents() const
{
  return m_withTargetLength ? 2 : 1;
}
  
std::string SpanLengthFeature::GetScoreProducerDescription(unsigned id) const
{
  return "SpanLength";
}

std::string SpanLengthFeature::GetScoreProducerWeightShortName(unsigned id) const
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
  SpanLengthFeatureState(unsigned terminalCount)
  : m_terminalCount(terminalCount)
  {
  }
  
  unsigned GetTerminalCount() const {
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
    const SpanLengthFeatureState* state = dynamic_cast<const SpanLengthFeatureState*>((*iter)->GetFFState(featureId));
    if (state != NULL) {
      spanInfo.TargetSpan = state->GetTerminalCount();
    }
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
  const SpanLengthEstimatorCollection& spanLengthEstimators = targetPhrase.GetSpanLengthEstimators();
  std::vector<float> scores(GetNumScoreComponents());
  //MARIA -> if ISI use only LHS rule span and ignore the rest at the moment
  bool useISI = (StaticData::Instance().GetParam("isi-format-for-span-length").size() > 0);
  if(useISI){
    size_t spanIndex=0; //only the entire LHS span -> in the rule table first tuple (N,M,S) will be the statistics of the LHS
    if(spans.size()>0)
      scores[0]=spanLengthEstimators.GetScoreBySourceSpanLength(spanIndex,chartHypothesis.GetCurrSourceRange().GetNumWordsCovered());
  }
  else{
    for (size_t spanIndex = 0; spanIndex < spans.size(); ++spanIndex) {
      scores[0] += spanLengthEstimators.GetScoreBySourceSpanLength(spanIndex, spans[spanIndex].SourceSpan);
    }
 }
  if (m_withTargetLength) {
    unsigned terminalCount = (unsigned)chartHypothesis.GetCurrTargetPhrase().GetSize();
    for (size_t spanIndex = 0; spanIndex < spans.size(); ++spanIndex) {
      terminalCount += spans[spanIndex].TargetSpan - 1;
      scores[1] += spanLengthEstimators.GetScoreByTargetSpanLength(spanIndex, spans[spanIndex].TargetSpan);
    }
    accumulator->PlusEquals(this, scores);
    return new SpanLengthFeatureState(terminalCount);
  } else {
    accumulator->PlusEquals(this, scores);
    return NULL;
  }
}
  
} // namespace moses
