#ifndef moses_PhraseLengthFeature_h
#define moses_PhraseLengthFeature_h

#include <stdexcept>
#include <string>
#include <map>

#include "StatelessFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/Word.h"
#include "moses/FactorCollection.h"

namespace Moses
{

/** Sets the features for length of source phrase, target phrase, both.
 */
class PhraseLengthFeature : public StatelessFeatureFunction
{
public:
  PhraseLengthFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}

  void EvaluateChart(const ChartHypothesis& hypo,
                     ScoreComponentCollection*) const {
    throw std::logic_error("PhraseLengthFeature not valid in chart decoder");
  }

  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

};

}

#endif // moses_PhraseLengthFeature_h
