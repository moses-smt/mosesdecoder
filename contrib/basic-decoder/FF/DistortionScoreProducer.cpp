
#include <boost/functional/hash.hpp>
#include "DistortionScoreProducer.h"
#include "TypeDef.h"
#include "Sentence.h"
#include "WordsBitmap.h"
#include "Search/Hypothesis.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////////////////////////////////////

DistortionScoreProducer::DistortionScoreProducer(const std::string &line)
  :StatefulFeatureFunction(line)
{
  ReadParameters();
}

size_t DistortionScoreProducer::Evaluate(const Hypothesis& hypo,
    size_t prevState,
    Scores &scores) const
{
  const WordsRange &range = hypo.GetRange();
  const WordsBitmap &coverage = hypo.GetCoverage();

  const Hypothesis *prevHypo = hypo.GetPrevHypo();
  assert(prevHypo);
  const WordsRange &prevRange = prevHypo->GetRange();

  SCORE score = ComputeDistortionScore(prevRange, range);
  scores.Add(*this, score);

  size_t firstGap = coverage.GetFirstGapPos();

  size_t ret = range.GetHash();
  boost::hash_combine(ret, firstGap);

  return ret;
}

SCORE DistortionScoreProducer::ComputeDistortionScore(const WordsRange &prev, const WordsRange &curr) const
{
  SCORE ret = (SCORE) prev.ComputeDistortionScore(curr);
  return ret;
}

SCORE DistortionScoreProducer::CalculateDistortionScore_MooreAndQuick(const Hypothesis& hypo,
    const WordsRange &prev, const WordsRange &curr, int firstGap)
{
  /* Pay distortion score as soon as possible, from Moore and Quirk MT Summit 2007
     Definitions:
     S   : current source range
     S'  : last translated source phrase range
     S'' : longest fully-translated initial segment
  */

  int prefixEndPos = (int)firstGap-1;
  if((int)firstGap==-1)
    prefixEndPos = -1;

  // case1: S is adjacent to S'' => return 0
  if ((int) curr.startPos == prefixEndPos+1) {
    return 0;
  }

  // case2: S is to the left of S' => return 2(length(S))
  if ((int) curr.endPos < (int) prev.endPos) {
    return (SCORE) -2*(int)curr.GetNumWordsCovered();
  }

  // case3: S' is a subsequence of S'' => return 2(nbWordBetween(S,S'')+length(S))
  if ((int) prev.endPos <= prefixEndPos) {
    int z = (int)curr.startPos-prefixEndPos - 1;
    return (SCORE) -2*(z + (int)curr.GetNumWordsCovered());
  }

  // case4: otherwise => return 2(nbWordBetween(S,S')+length(S))
  return (SCORE) -2*((int)curr.GetNumWordsBetween(prev) + (int)curr.GetNumWordsCovered());


}
