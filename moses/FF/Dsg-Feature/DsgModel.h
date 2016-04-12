#pragma once

#include <string>
#include <map>
#include <vector>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/Manager.h"
#include "moses/FF/Dsg-Feature/dsgHyp.h"
#include "moses/FF/Dsg-Feature/Desegmenter.h"
#include "KenDsg.h"


namespace Moses
{

class DesegModel : public StatefulFeatureFunction
{
public:

  DsgLM * DSGM;
  Desegmenter* desegT;
  int tFactor;// Target Factor ...
  int order;
  int numFeatures;   // Number of features used an be 1 (unsegmented LM)or 5 (with 3 contiguity features and 1 UnsegWP)
  bool optimistic;

  DesegModel(const std::string &line);
  ~DesegModel();

  void readLanguageModel(const char *);
  void Load(AllOptions::ptr const& opts);

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  virtual FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void  EvaluateInIsolation(const Phrase &source
                            , const TargetPhrase &targetPhrase
                            , ScoreComponentCollection &scoreBreakdown
                            , ScoreComponentCollection &estimatedScores) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual std::string GetScoreProducerWeightShortName(unsigned idx=0) const;

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const;

protected:
  typedef std::vector<float> Scores;
  std::string m_lmPath;
  std::string m_desegPath;
  bool m_simple; //desegmentation scheme; if 1 then use simple, else use rule and backoff to simple
};


}
