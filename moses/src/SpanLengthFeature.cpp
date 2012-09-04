#include "SpanLengthFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"
#include "WordsRange.h"
#include "ChartHypothesis.h"
#include "types.h"

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
  
static std::vector<int> GetPrevHyposSourceLengths(const ChartHypothesis &chartHypothesis)
{
  const std::vector<const ChartHypothesis*>& prevHypos = chartHypothesis.GetPrevHypos();
  std::vector<WordsRange> prevHyposRanges;
  prevHyposRanges.reserve(prevHypos.size());
  iterate(prevHypos, iter) {
    prevHyposRanges.push_back((*iter)->GetCurrSourceRange());
  }
  std::sort(prevHyposRanges.begin(), prevHyposRanges.end());
  std::vector<int> result;
  result.reserve(prevHypos.size());
  iterate(prevHyposRanges, iter) {
    result.push_back(static_cast<int>(iter->GetNumWordsCovered()));
  }
  return result;
}

FFState* SpanLengthFeature::EvaluateChart(
  const ChartHypothesis &chartHypothesis,
  int /*featureId*/,
  ScoreComponentCollection *accumulator) const
{
  std::vector<int> sourceLengths = GetPrevHyposSourceLengths(chartHypothesis);
  const TargetPhrase& targetPhrase = chartHypothesis.GetCurrTargetPhrase();
  for (size_t spanIndex = 0; spanIndex < sourceLengths.size(); ++spanIndex) {
    float weight = 0.0f;
    accumulator->PlusEquals(this, weight);
  }
  return NULL;
}
  
} // namespace moses