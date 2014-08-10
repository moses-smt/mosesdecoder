#ifndef moses_CorrectionPattern_h
#define moses_CorrectionPattern_h

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
class CorrectionPattern : public StatelessFeatureFunction
{
private:
  boost::unordered_set<std::string> m_vocab;
  FactorType m_factorType;
  bool m_unrestricted;
  std::string m_filename;
  size_t m_top;

public:
  CorrectionPattern(const std::string &line);

  bool IsUseable(const FactorMask &mask) const;

  void Load();

  void EvaluateInIsolation(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const
  {}
  
  virtual void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  
  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}

  void ComputeFeatures(const InputType &input,
                       const InputPath &inputPath,
                       const TargetPhrase& targetPhrase,
                       ScoreComponentCollection* accumulator) const;
  
  void SetParameter(const std::string& key, const std::string& value);
  
  std::vector<std::string> CreatePattern(const Tokens &s1,
                                         const Tokens &s2,
                                         const InputType &input,
                                         const InputPath &inputPath) const;
  
  std::string CreateSinglePattern(const Tokens &s1, const Tokens &s2) const;

};

}

#endif // moses_CorrectionPattern_h
