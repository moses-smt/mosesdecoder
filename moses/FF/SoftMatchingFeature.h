#pragma once

#include "moses/Word.h"
#include "StatelessFeatureFunction.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace Moses
{

class SoftMatchingFeature : public StatelessFeatureFunction
{
public:
  SoftMatchingFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void EvaluateWhenApplied(const ChartHypothesis& hypo,
                             ScoreComponentCollection* accumulator) const;

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const {};

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                , const TranslationOptionList &translationOptionList) const
  {}

  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const {};

  bool Load(const std::string &filePath);

  std::vector<std::vector<Word> >& GetSoftMatches() {
    return m_softMatches;
  }

  void ResizeCache() const;

  const std::string& GetOrSetFeatureName(const Word& RHS, const Word& LHS) const;
  void SetParameter(const std::string& key, const std::string& value);


private:
  mutable std::vector<std::vector<Word> > m_softMatches; // map RHS of new rule to list of possible LHS of old rule (subtree)
  mutable std::vector<std::vector<std::string> > m_nameCache;

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

};

}

