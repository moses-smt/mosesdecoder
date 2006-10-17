// $Id$

#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"

DistortionScoreProducer::DistortionScoreProducer()
{
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
}

size_t DistortionScoreProducer::GetNumScoreComponents() const
{
	return 1;
}

const std::string DistortionScoreProducer::GetScoreProducerDescription() const
{
	return "distortion score";
}

float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr) const
{
  if (prev.GetWordsCount() == 0)
  { // 1st hypothesis with translated phrase. NOT the seed hypo.
    return - (float) curr.GetStartPos();
  }
  else
  { // add distortion score of current translated phrase to
    // distortions scores of all previous partial translations
    return - (float) curr.CalcDistortion(prev);
	}
}

WordPenaltyProducer::WordPenaltyProducer()
{
	const_cast<ScoreIndexManager&>(StaticData::Instance()->GetScoreIndexManager()).AddScoreProducer(this);
}

size_t WordPenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

const std::string WordPenaltyProducer::GetScoreProducerDescription() const
{
	return "word penalty";
}


size_t UnknownWordPenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

const std::string UnknownWordPenaltyProducer::GetScoreProducerDescription() const
{
	return "unknown word penalty";
}

