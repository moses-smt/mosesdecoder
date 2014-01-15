#ifndef moses_SourceWordDeletionFeature_h
#define moses_SourceWordDeletionFeature_h

#include <string>
#include <boost/unordered_set.hpp>

#include "StatelessFeatureFunction.h"
#include "moses/FactorCollection.h"
#include "moses/AlignmentInfo.h"

namespace Moses
{

/** Sets the features for source word deletion
 */
class SourceWordDeletionFeature : public StatelessFeatureFunction
{
private:
  boost::unordered_set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;
  std::string m_filename;

public:
  SourceWordDeletionFeature(const std::string &line);

  void Load();

  bool IsUseable(const FactorMask &mask) const;

  void Evaluate(const Phrase &source
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
                       ScoreComponentCollection* accumulator,
                       const AlignmentInfo &alignmentInfo) const;
  void SetParameter(const std::string& key, const std::string& value);

};

}

#endif // moses_SourceWordDeletionFeature_h
