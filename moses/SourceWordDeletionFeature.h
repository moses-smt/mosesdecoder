#ifndef moses_SourceWordDeletionFeature_h
#define moses_SourceWordDeletionFeature_h

#include <string>
#include <map>

#include "FeatureFunction.h"
#include "FactorCollection.h"
#include "AlignmentInfo.h"

namespace Moses
{

/** Sets the features for source word deletion
 */
class SourceWordDeletionFeature : public StatelessFeatureFunction {
private:
  std::set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;

public:
	SourceWordDeletionFeature(FactorType factorType = 0):
     StatelessFeatureFunction("swd", ScoreProducer::unlimited),
     m_factorType(factorType),
     m_unrestricted(true)
  {
		std::cerr << "Initializing source word deletion feature.." << std::endl;
  }
      
  bool Load(const std::string &filePath);
  void Evaluate(const PhraseBasedFeatureContext& context, 
              ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const ChartBasedFeatureContext& context,
  		               ScoreComponentCollection* accumulator) const;

  void ComputeFeatures(const TargetPhrase& targetPhrase, 
		  	           ScoreComponentCollection* accumulator, 
		  	           const AlignmentInfo &alignmentInfo) const;

  // basic properties
	std::string GetScoreProducerWeightShortName(unsigned) const { return "swd"; }
	size_t GetNumInputScores() const { return 0; }
};

}

#endif // moses_SourceWordDeletionFeature_h
