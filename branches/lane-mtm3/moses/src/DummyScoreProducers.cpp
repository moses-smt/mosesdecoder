// $Id$

#include <cassert>
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "WordsRange.h"

namespace Moses
{

WERScoreProducer::WERScoreProducer(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

std::string WERScoreProducer::GetScoreProducerDescription() const
{
	return "WERScoreProducer";
}

float WERScoreProducer::CalculateScore(const Phrase &hypPhrase, const Phrase &constraintPhrase) const
{
	const size_t len1 = hypPhrase.GetSize(), len2 = constraintPhrase.GetSize();
	vector<vector<unsigned int> > d(len1 + 1, vector<unsigned int>(len2 + 1));
	
	for(int i = 0; i <= len1; ++i) d[i][0] = i;
	for(int i = 0; i <= len2; ++i) d[0][i] = i;
	
	for(int i = 1; i <= len1; ++i)
	{
		for(int j = 1; j <= len2; ++j) {
			WordsRange s1range(i-1, i-1);
			WordsRange s2range(j-1, j-1);
			int cost = hypPhrase.GetSubString(s1range).IsCompatible(constraintPhrase.GetSubString(s2range)) ? 0 : 1;
			d[i][j] = std::min( std::min(d[i - 1][j] + 1,
										 d[i][j - 1] + 1),
							   d[i - 1][j - 1] + cost);
		}
	}
	return d[len1][len2];
}

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


