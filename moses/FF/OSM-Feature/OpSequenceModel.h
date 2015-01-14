#pragma once

#include <string>
#include <map>
#include <vector>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/Manager.h"
#include "moses/FF/OSM-Feature/osmHyp.h"
#include "KenOSM.h"

namespace Moses
{

class OpSequenceModel : public StatefulFeatureFunction
{
public:

  OSMLM* OSM;
  float unkOpProb;
  int sFactor;	// Source Factor ...
  int tFactor;	// Target Factor ...
  int numFeatures;   // Number of features used ...

  OpSequenceModel(const std::string &line);
  ~OpSequenceModel();

  void readLanguageModel(const char *);
  void Load();

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

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

  void  EvaluateInIsolation(const Phrase &source
                            , const TargetPhrase &targetPhrase
                            , ScoreComponentCollection &scoreBreakdown
                            , ScoreComponentCollection &estimatedFutureScore) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual std::string GetScoreProducerWeightShortName(unsigned idx=0) const;

  std::vector<float> GetFutureScores(const Phrase &source, const Phrase &target) const;
  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const;

protected:
  typedef std::pair<Phrase, Phrase> ParallelPhrase;
  typedef std::vector<float> Scores;
  std::map<ParallelPhrase, Scores> m_futureCost;

  std::vector < std::pair < std::set <int> , std::set <int> > > ceptsInPhrase;
  std::set <int> targetNullWords;
  std::string m_lmPath;


};


} // namespace
