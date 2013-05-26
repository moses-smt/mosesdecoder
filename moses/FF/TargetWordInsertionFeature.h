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
class TargetWordInsertionFeature : public StatelessFeatureFunction {
private:
  boost::unordered_set<std::string> m_vocab;
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

};

}

#endif // moses_TargetWordInsertionFeature_h
