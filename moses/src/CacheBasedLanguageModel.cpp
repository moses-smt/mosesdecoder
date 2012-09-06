// $Id$

#include "util/check.hh"
#include "StaticData.h"
#include "CacheBasedLanguageModel.h"
//#include "WordsRange.h"
//#include "TranslationOption.h"

namespace Moses
{

CacheBasedLanguageModel::CacheBasedLanguageModel(ScoreIndexManager &scoreIndexManager, const std::vector<float>& weights)
{
  const_cast<ScoreIndexManager&>  (StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

size_t CacheBasedLanguageModel::GetNumScoreComponents() const
{
  return 1;
}

std::string CacheBasedLanguageModel::GetScoreProducerDescription(unsigned) const
{
  return "CacheBasedLanguageModel";
}

std::string CacheBasedLanguageModel::GetScoreProducerWeightShortName(unsigned) const
{
  return "cblm";
}

void CacheBasedLanguageModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
  float score = 0.1;
  out->PlusEquals(this, score);
}

}
