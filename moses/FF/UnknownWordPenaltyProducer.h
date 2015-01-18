#pragma once

// $Id$

#include "StatelessFeatureFunction.h"

namespace Moses
{

class WordsRange;


/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
protected:
  static UnknownWordPenaltyProducer *s_instance;

public:
  static const UnknownWordPenaltyProducer& Instance() {
    return *s_instance;
  }
  static UnknownWordPenaltyProducer& InstanceNonConst() {
    return *s_instance;
  }

  UnknownWordPenaltyProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  std::vector<float> DefaultWeights() const;

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWhenApplied(const Syntax::SHyperedge &hyperedge,
                           ScoreComponentCollection* accumulator) const {
  }
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }

};

}

