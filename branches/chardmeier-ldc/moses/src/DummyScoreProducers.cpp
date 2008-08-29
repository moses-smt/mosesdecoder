// $Id$

#include <cassert>
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

float beta_binomial(float p, float q, int x);

#define DISTORTION_RANGE 6
float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, float px, float qx) const
{
        // float p = px + 2.23;
        float p = px + 2;
        float q = qx + 2;

        int x = StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);
        if(x < -DISTORTION_RANGE) x = -DISTORTION_RANGE;
        if(x >  DISTORTION_RANGE) x =  DISTORTION_RANGE;
        x += DISTORTION_RANGE;

        return beta_binomial(p, q, x);
}

float beta_binomial(float p, float q, int x)
{
        const long double n = (long double) 2 * DISTORTION_RANGE;

        long double n1 = lgammal(n + 1);
        long double n2 = lgammal(q + n - x);
        long double n3 = lgammal(x + p);
        long double n4 = lgammal(p + q);
        long double d1 = lgammal(x + 1);
        long double d2 = lgammal(n - x + 1);
        long double d3 = lgammal(p + q + n);
        long double d4 = lgammal(p);
        long double d5 = lgammal(q);

        // std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
        // std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;

        long double part1 = n1 + n4 - d3 - d4 - d5;
        // std::cerr << part1 << std::endl;
        long double part2 = n2 + n3 - d1 - d2;
        // std::cerr << part2 << std::endl;

        long double score = part1 + part2;
        // std::cerr << "x: " << x << "   score: " << score << std::endl;

        if(!finite(score)) {
                std::cerr << p << " ; " << q << std::endl;
                std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
                std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;
                std::cerr << part1 << std::endl;
                std::cerr << part2 << std::endl;
                std::cerr << "x: " << x << "   score: " << score << std::endl;
                assert(false);
        }

        return FloorScore((float) score);
}

/*** OLD CODE ***/
#if 0
float DistortionScoreProducer::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr, const int FirstGap) const
{
  const int USE_OLD = 1;
  if (USE_OLD) {
	return - abs((float) StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr));
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
#endif


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

