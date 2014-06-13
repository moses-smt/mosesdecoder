#ifndef moses_EditOps_h
#define moses_EditOps_h

#include <string>
#include <boost/unordered_set.hpp>

#include "StatelessFeatureFunction.h"
#include "moses/FactorCollection.h"
#include "moses/AlignmentInfo.h"

namespace Moses
{

typedef std::vector<std::string> Tokens;

/** Sets the features for length of source phrase, target phrase, both.
 */
class EditOps : public StatelessFeatureFunction
{
private:
  FactorType m_factorType;
  bool m_chars;

public:
  EditOps(const std::string &line);

  bool IsUseable(const FactorMask &mask) const;

  void Load();

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}

  void ComputeFeatures(const Phrase &source,
                       const TargetPhrase& targetPhrase,
                       ScoreComponentCollection* accumulator) const;
  void SetParameter(const std::string& key, const std::string& value);
};

}

#endif // moses_CorrectionPattern_h
