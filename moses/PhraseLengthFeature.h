#ifndef moses_PhraseLengthFeature_h
#define moses_PhraseLengthFeature_h

#include <stdexcept>
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
  PhraseLengthFeature(const std::string &line);
      
  void Evaluate(const PhraseBasedFeatureContext& context,
                ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const ChartBasedFeatureContext& context,
                     ScoreComponentCollection*) const {
    throw std::logic_error("PhraseLengthFeature not valid in chart decoder");
	}

};

}

#endif // moses_PhraseLengthFeature_h
