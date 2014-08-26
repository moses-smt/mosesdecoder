#pragma once

#include <string>
#include <map>
#include <vector>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/Manager.h"
#include "KenESM.h"

namespace Moses
{

class esmState : public FFState                                                                                                                                                                                                
{                                                                                                                                                                                                                              
public:                                                                                                                                                                                                                        
  esmState(const lm::ngram::State& val);                                                                                                                                                                                      

  int Compare(const FFState& other) const;                                                                                                                                                                                    
 
  lm::ngram::State getLMState() const {                                                                                                                                                                                        
    return lmState;                                                                                                                                                                                                            
  }
  
  void setLMState(const lm::ngram::State& val);
 
  void print() const;                                                                                                                                                                                                          
  std::string getName() const;                                                                                                                                                                                                 
 
protected:                                                                                                                                                                                                                     
  lm::ngram::State lmState;                                                                                                                                                                                                    
};

class EditSequenceModel : public StatefulFeatureFunction
{
public:

  ESMLM* ESM;
  float unkOpProb;
  int m_sFactor;    // Source Factor ...
  int m_tFactor;    // Target Factor ...
  int numFeatures;   // Number of features used ...

  EditSequenceModel(const std::string &line);
  ~EditSequenceModel();

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
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}
  
  void  EvaluateInIsolation(const Phrase &source
                 , const TargetPhrase &targetPhrase
                 , ScoreComponentCollection &scoreBreakdown
                 , ScoreComponentCollection &estimatedFutureScore) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual std::string GetScoreProducerWeightShortName(unsigned idx=0) const;

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const;

protected:
  float calculateScore(const std::vector<std::string>&, const FFState*, FFState*, bool) const;
  
  typedef std::vector<float> Scores;
  std::string m_lmPath;
};


} // namespace
