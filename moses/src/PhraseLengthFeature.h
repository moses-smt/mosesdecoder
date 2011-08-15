#ifndef moses_PhraseLengthFeature_h
#define moses_PhraseLengthFeature_h

#include <string>
#include <map>

#include "FactorCollection.h"
#include "FeatureFunction.h"
#include "FFState.h"
#include "Word.h"

namespace Moses
{

/** Sets the features for length of source phrase, target phrase, both.
 */
class PhraseLengthFeature : public StatelessFeatureFunction {
public:
	PhraseLengthFeature():
     StatelessFeatureFunction("pl")
  {}
      
  void Evaluate(const TargetPhrase& cur_phrase,
                ScoreComponentCollection* accumulator) const;

  // basic properties
	size_t GetNumScoreComponents() const { return ScoreProducer::unlimited; }
	std::string GetScoreProducerWeightShortName() const { return "pl"; }
	size_t GetNumInputScores() const { return 0; }
};

}

#endif // moses_PhraseLengthFeature_h
