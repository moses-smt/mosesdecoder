#include "SpanLengthFeature.h"
#include "util/check.hh"
#include "FFState.h"
#include "TranslationOption.h"

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

FFState* SpanLengthFeature::EvaluateChart(
  const ChartHypothesis &/*char_hypothesis*/,
  int /*featureId*/,
  ScoreComponentCollection */*accumulator*/) const
{
  return NULL;
}
  
} // namespace moses