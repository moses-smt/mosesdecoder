#include <iostream>
#include "SparsePhraseDictionaryFeature.h"

using namespace std;

namespace Moses 
{

SparsePhraseDictionaryFeature::SparsePhraseDictionaryFeature(const std::string &line)
:StatelessFeatureFunction("SparsePhraseDictionaryFeature", FeatureFunction::unlimited, line)
{
}

void SparsePhraseDictionaryFeature::Evaluate(
            const PhraseBasedFeatureContext& context,
            ScoreComponentCollection* accumulator) const
{
  //not used
}



}
