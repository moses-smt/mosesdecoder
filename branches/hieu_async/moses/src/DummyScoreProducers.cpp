// $Id: DummyScoreProducers.cpp 98 2007-09-17 21:02:40Z hieu $

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

const std::string DistortionScoreProducer::GetScoreProducerDescription(int idx) const
{
	return "distortion score";
}

float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr) const
{
  // add distortion score of current translated phrase to
  // distortions scores of all previous partial translations
  return - (float) curr.CalcDistortion(prev);
}

WordPenaltyProducer::WordPenaltyProducer(ScoreIndexManager &scoreIndexManager, float wpWeight)
:m_wpWeight(wpWeight)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t WordPenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

const std::string WordPenaltyProducer::GetScoreProducerDescription(int idx) const
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

const std::string UnknownWordPenaltyProducer::GetScoreProducerDescription(int idx) const
{
	return "unknown word penalty";
}

