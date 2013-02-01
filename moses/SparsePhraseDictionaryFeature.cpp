#include "SparsePhraseDictionaryFeature.h"


namespace Moses 
{

SparsePhraseDictionaryFeature::SparsePhraseDictionaryFeature(const std::string &line)
:StatelessFeatureFunction("SparsePhraseDictionaryFeature", ScoreProducer::unlimited, line)
{
}

void SparsePhraseDictionaryFeature::Evaluate(
            const PhraseBasedFeatureContext& context,
            ScoreComponentCollection* accumulator) const
{
  //not used
}



}
