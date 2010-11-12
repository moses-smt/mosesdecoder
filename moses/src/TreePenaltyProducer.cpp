#include <cassert>
#include "FFState.h"
#include "StaticData.h"
#include "TreePenaltyProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"

namespace Moses
{

TreePenaltyProducer::TreePenaltyProducer(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t TreePenaltyProducer::GetNumScoreComponents() const
{
	return 1;
}

std::string TreePenaltyProducer::GetScoreProducerDescription() const
{
	return "TreePenalty";
}

std::string TreePenaltyProducer::GetScoreProducerWeightShortName() const
{
	return "tp";
}

size_t TreePenaltyProducer::GetNumInputScores() const { return 0;}

/*void TreePenaltyProducer::Evaluate(ScoreComponentCollection* out) const
{
  out->PlusEquals(this, m_score);
}*/

}
