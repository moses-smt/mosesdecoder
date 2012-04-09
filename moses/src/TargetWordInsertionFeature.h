#ifndef moses_TargetWordInsertionFeature_h
#define moses_TargetWordInsertionFeature_h

#include <string>
#include <map>

#include "FeatureFunction.h"
#include "FactorCollection.h"

namespace Moses
{

/** Sets the features for length of source phrase, target phrase, both.
 */
class TargetWordInsertionFeature : public StatelessFeatureFunction {
private:
  std::set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;

public:
	TargetWordInsertionFeature(FactorType factorType = 0):
     StatelessFeatureFunction("twi", ScoreProducer::unlimited),
     m_factorType(factorType),
     m_unrestricted(true)
  {}
      
  bool Load(const std::string &filePath);
  void Evaluate(const ChartHypothesis& cur_hypo,
                ScoreComponentCollection* accumulator) const;

  void EvaluateChart(
    const ChartHypothesis& cur_hypo,
    int featureID,
    ScoreComponentCollection* accumulator) const;

  void ComputeFeatures(const TargetPhrase& targetPhrase,
  		 	 	 	 	 	 	 	 	 ScoreComponentCollection* accumulator) const;

  // basic properties
	std::string GetScoreProducerWeightShortName(unsigned) const { return "twi"; }
	size_t GetNumInputScores() const { return 0; }
};

}

#endif // moses_TargetWordInsertionFeature_h
