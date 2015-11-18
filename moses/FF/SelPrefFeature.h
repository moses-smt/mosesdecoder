#ifndef moses_SelPrefFeature_h
#define moses_SelPrefFeature_h

#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Word.h"


namespace Moses{

class SelPrefFeature : public StatefulFeatureFunction{

public:

  SelPrefFeature(const std::string &line);
  ~SelPrefFeature(){};

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
      return NULL;
    }

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
        , const TranslationOptionList &translationOptionList) const {
    }

  FFState* EvaluateWhenApplied(
      const Hypothesis& cur_hypo,
      const FFState* prev_state,
      ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};

  FFState* EvaluateWhenApplied(
      const ChartHypothesis& /* cur_hypo */,
      int /* featureID - used to index the state in the previous hypotheses */,
      ScoreComponentCollection* accumulator) const;

  void Load();

  void CleanUpAfterSentenceProcessing(const InputType& source);



};

}
#endif
