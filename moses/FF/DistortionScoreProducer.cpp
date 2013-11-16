
#include "DistortionScoreProducer.h"
#include "FFState.h"
#include "moses/WordsRange.h"
#include "moses/StaticData.h"
#include "moses/Hypothesis.h"

using namespace std;

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

DistortionScoreProducer::DistortionScoreProducer(const std::string &line)
  : StatefulFeatureFunction(1, line)
{
  ReadParameters();
}

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

float DistortionScoreProducer::CalculateDistortionScore(const Hypothesis& hypo,
    const WordsRange &prev, const WordsRange &curr, const int FirstGap)
{
  if(!StaticData::Instance().UseEarlyDistortionCost()) {
    return - (float) hypo.GetInput().ComputeDistortionDistance(prev, curr);
  } else {
    /* Pay distortion score as soon as possible, from Moore and Quirk MT Summit 2007
       Definitions:
       S   : current source range
       S'  : last translated source phrase range
       S'' : longest fully-translated initial segment
    */

    int prefixEndPos = (int)FirstGap-1;
    if((int)FirstGap==-1)
      prefixEndPos = -1;

    // case1: S is adjacent to S'' => return 0
    if ((int) curr.GetStartPos() == prefixEndPos+1) {
      IFVERBOSE(4) std::cerr<< "MQ07disto:case1" << std::endl;
      return 0;
    }

    // case2: S is to the left of S' => return 2(length(S))
    if ((int) curr.GetEndPos() < (int) prev.GetEndPos()) {
      IFVERBOSE(4) std::cerr<< "MQ07disto:case2" << std::endl;
      return (float) -2*(int)curr.GetNumWordsCovered();
    }

    // case3: S' is a subsequence of S'' => return 2(nbWordBetween(S,S'')+length(S))
    if ((int) prev.GetEndPos() <= prefixEndPos) {
      IFVERBOSE(4) std::cerr<< "MQ07disto:case3" << std::endl;
      int z = (int)curr.GetStartPos()-prefixEndPos - 1;
      return (float) -2*(z + (int)curr.GetNumWordsCovered());
    }

    // case4: otherwise => return 2(nbWordBetween(S,S')+length(S))
    IFVERBOSE(4) std::cerr<< "MQ07disto:case4" << std::endl;
    return (float) -2*((int)curr.GetNumWordsBetween(prev) + (int)curr.GetNumWordsCovered());

  }
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
    hypo.GetWordsBitmap().GetFirstGapPos());
  return res;
}


}

