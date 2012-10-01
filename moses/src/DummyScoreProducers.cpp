// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"
#include "TranslationOption.h"

namespace Moses
{

struct DistortionState_traditional : public FFState {
  WordsRange range;
  int first_gap;
  DistortionState_traditional(const WordsRange& wr, int fg) : range(wr), first_gap(fg) {}
  int Compare(const FFState& other) const {
    const DistortionState_traditional& o =
      static_cast<const DistortionState_traditional&>(other);
    if (range.GetEndPos() < o.range.GetEndPos()) return -1;
    if (range.GetEndPos() > o.range.GetEndPos()) return 1;
    return 0;
  }
};

const FFState* DistortionScoreProducer::EmptyHypothesisState(const InputType &input) const
{
  // fake previous translated phrase start and end
  size_t start = NOT_FOUND;
  size_t end = NOT_FOUND;
  if (input.m_frontSpanCoveredLength > 0) {
    // can happen with --continue-partial-translation
    start = 0;
    end = input.m_frontSpanCoveredLength -1;
  }
  return new DistortionState_traditional(
           WordsRange(start, end),
           NOT_FOUND);
}


std::string DistortionScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "d";
}

float DistortionScoreProducer::CalculateDistortionScore(const Hypothesis& hypo,
    const WordsRange &prev, const WordsRange &curr, const int FirstGap) const
{
  const int USE_OLD = 1;
  if (USE_OLD) {
    return - (float) hypo.GetInput().ComputeDistortionDistance(prev, curr);
  }

  // Pay distortion score as soon as possible, from Moore and Quirk MT Summit 2007

  int prefixEndPos = FirstGap-1;
  if ((int) curr.GetStartPos() == prefixEndPos+1) {
    return 0;
  }

  if ((int) curr.GetEndPos() < (int) prev.GetEndPos()) {
    return (float) -2*curr.GetNumWordsCovered();
  }

  if ((int) prev.GetEndPos() <= prefixEndPos) {
    int z = curr.GetStartPos()-prefixEndPos;
    return (float) -2*(z + curr.GetNumWordsCovered());
  }

  return (float) -2*(curr.GetNumWordsBetween(prev) + curr.GetNumWordsCovered());
}


FFState* DistortionScoreProducer::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* out) const
{
  const DistortionState_traditional* prev = static_cast<const DistortionState_traditional*>(prev_state);
  const float distortionScore = CalculateDistortionScore(
                                  hypo,
                                  prev->range,
                                  hypo.GetCurrSourceWordsRange(),
                                  prev->first_gap);
  out->PlusEquals(this, distortionScore);
  DistortionState_traditional* res = new DistortionState_traditional(
    hypo.GetCurrSourceWordsRange(),
    hypo.GetPrevHypo()->GetWordsBitmap().GetFirstGapPos());
  return res;
}


std::string WordPenaltyProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "w";
}

void WordPenaltyProducer::Evaluate(const Hypothesis& cur_hypo, ScoreComponentCollection* out) const
{
	const TargetPhrase& tp = cur_hypo.GetCurrTargetPhrase();
  out->PlusEquals(this, -static_cast<float>(tp.GetSize()));
}

std::string UnknownWordPenaltyProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "u";
}


bool UnknownWordPenaltyProducer::ComputeValueInTranslationOption() const
{
  return true;
}

std::string MetaFeatureProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "m"+m_shortName;
}

}
