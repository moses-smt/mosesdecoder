#ifndef moses_TargetWordInsertionFeature_h
#define moses_TargetWordInsertionFeature_h

#include <string>
#include <map>

#include "FeatureFunction.h"
#include "FactorCollection.h"
#include "AlignmentInfo.h"

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
  TargetWordInsertionFeature(const std::string &line);
      
  bool Load(const std::string &filePath);

  virtual void Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedFutureScore) const;

  void ComputeFeatures(const TargetPhrase& targetPhrase,
		           	   ScoreComponentCollection* accumulator,
		           	   const AlignmentInfo &alignmentInfo) const;

  virtual StatelessFeatureType GetStatelessFeatureType() const
  { return RequiresTargetPhrase; }

};

}

#endif // moses_TargetWordInsertionFeature_h
