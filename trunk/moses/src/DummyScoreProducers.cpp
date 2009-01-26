// $Id$

#include <cassert>
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"

namespace Moses
{
DistortionScoreProducer::DistortionScoreProducer(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t DistortionScoreProducer::GetNumScoreComponents() const
{
	return 1;
}

std::string DistortionScoreProducer::GetScoreProducerDescription() const
{
	return "Distortion";
}

//float DistortionScoreProducer::CalculateDistortionScoreOUTDATED(const WordsRange &prev, const WordsRange &curr) const
//{
//	return - (float) StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);
//}

float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, const int FirstGap) const
{
  const int USE_OLD = 1;
  if (USE_OLD) {
	return - (float) StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);
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



WordPenaltyProducer::WordPenaltyProducer(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t WordPenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

std::string WordPenaltyProducer::GetScoreProducerDescription() const
{
	return "WordPenalty";
}

UnknownWordPenaltyProducer::UnknownWordPenaltyProducer(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t UnknownWordPenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

std::string UnknownWordPenaltyProducer::GetScoreProducerDescription() const
{
	return "!UnknownWordPenalty";
}

}


