#include "SelPrefFeature.h"

#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/StaticData.h"
#include "moses/ChartHypothesis.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/TreeStructurePhraseProperty.h"

#include <string>


using namespace std;


namespace Moses
{

SelPrefFeature::SelPrefFeature(const std::string &line)
  :StatefulFeatureFunction(0, line){

	ReadParameters();
}

void SelPrefFeature::SetParameter(const std::string& key, const std::string& value){

}

void SelPrefFeature::Load() {

  StaticData &staticData = StaticData::InstanceNonConst();
  //staticData.SelPrefFeature(this);
}

// For clearing caches and counters

void SelPrefFeature::CleanUpAfterSentenceProcessing(const InputType& source){
}


FFState* SelPrefFeature::EvaluateWhenApplied(
  const ChartHypothesis&  cur_hypo,
  int  featureID /*- used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	return nullptr;
}


}
