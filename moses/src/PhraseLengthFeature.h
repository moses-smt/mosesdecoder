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
     StatelessFeatureFunction("pl", ScoreProducer::unlimited)
  {}
      
  void Evaluate(const TranslationOption& translationOption,
                const InputType& inputType,
                const WordsBitmap& coverageVector,
                ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const TargetPhrase& targetPhrase,
                     const InputType& inputType,
                     const WordsRange& sourceSpan,
                     ScoreComponentCollection*) const {
		CHECK(0); // feature function not valid in chart decoder
	}

  // basic properties
	std::string GetScoreProducerWeightShortName(unsigned) const { return "pl"; }
	size_t GetNumInputScores() const { return 0; }
};

}

#endif // moses_PhraseLengthFeature_h
