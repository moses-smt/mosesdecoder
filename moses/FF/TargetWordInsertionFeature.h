#ifndef moses_TargetWordInsertionFeature_h
#define moses_TargetWordInsertionFeature_h

#include <string>
#include <boost/unordered_set.hpp>

#include "StatelessFeatureFunction.h"
#include "moses/FactorCollection.h"
#include "moses/AlignmentInfo.h"

namespace Moses
{

/** Sets the features for length of source phrase, target phrase, both.
 */
class TargetWordInsertionFeature : public StatelessFeatureFunction
{
private:
  boost::unordered_set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;
  std::string m_filename;

public:
  TargetWordInsertionFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const;

  void Load();

  virtual void EvaluateInIsolation(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}
  
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                , const TranslationOptionList &translationOptionList) const
  {}

  void ComputeFeatures(const Phrase &source,
                       const TargetPhrase& targetPhrase,
                       ScoreComponentCollection* accumulator,
                       const AlignmentInfo &alignmentInfo) const;
  void SetParameter(const std::string& key, const std::string& value);

};

}

#endif // moses_TargetWordInsertionFeature_h
