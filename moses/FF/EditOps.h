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

/** Calculates string edit operations that transform source phrase into target
 * phrase using the LCS algorithm. Potentially usefule for monolingual tasks
 * like paraphrasing, summarization, correction.
 */
class EditOps : public StatelessFeatureFunction
{
private:
  FactorType m_factorType;
  bool m_chars;
  std::string m_scores;

public:
  EditOps(const std::string &line);

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
                       ScoreComponentCollection* accumulator) const;
  void SetParameter(const std::string& key, const std::string& value);
};

}

#endif // moses_CorrectionPattern_h
