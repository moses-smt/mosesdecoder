// $Id$

#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"

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
	return "distortion score";
}

float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr) const
{
  if (prev.GetNumWordsCovered() == 0)
  { // 1st hypothesis with translated phrase. NOT the seed hypo.
    return - (float) curr.GetStartPos();
  }
  else
  { // add distortion score of current translated phrase to
    // distortions scores of all previous partial translations
    return - (float) curr.CalcDistortion(prev);
	}
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
	return "word penalty";
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
	return "unknown word penalty";
}

