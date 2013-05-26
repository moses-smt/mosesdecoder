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
class SourceWordDeletionFeature : public StatelessFeatureFunction {
private:
  boost::unordered_set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;

public:
  SourceWordDeletionFeature(const std::string &line);
      
  bool Load(const std::string &filePath);
  void Evaluate(const PhraseBasedFeatureContext& context, 
              ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const ChartBasedFeatureContext& context,
  		               ScoreComponentCollection* accumulator) const;

  virtual void Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedFutureScore) const;

  void ComputeFeatures(const TargetPhrase& targetPhrase, 
		  	           ScoreComponentCollection* accumulator, 
		  	           const AlignmentInfo &alignmentInfo) const;
};

}

#endif // moses_SourceWordDeletionFeature_h
