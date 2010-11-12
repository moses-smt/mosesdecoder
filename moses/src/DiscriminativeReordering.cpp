#include <cassert>
#include "FFState.h"
#include "StaticData.h"
#include "DiscriminativeReordering.h"
#include "WordsRange.h"
#include "TranslationOption.h"

namespace Moses
{

DiscriminativeReordering::DiscriminativeReordering(ScoreIndexManager &scoreIndexManager)
{
	scoreIndexManager.AddScoreProducer(this);
}

size_t DiscriminativeReordering::GetNumScoreComponents() const
{
	return 1;
}

std::string DiscriminativeReordering::GetScoreProducerDescription() const
{
	return "DiscriminativeReordering";
}

std::string DiscriminativeReordering::GetScoreProducerWeightShortName() const
{
	return "dis";
}

size_t DiscriminativeReordering::GetNumInputScores() const { return 0;}

/*void DiscriminativeReordering::Evaluate(ScoreComponentCollection* out) const
{
  out->PlusEquals(this, m_score);
}*/

}
