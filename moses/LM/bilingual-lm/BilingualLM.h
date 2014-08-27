#pragma once

#include <string>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include <boost/thread/tss.hpp>
#include "moses/Hypothesis.h"
#include "moses/InputPath.h"
#include "moses/Manager.h"

namespace nplm {
  class neuralLM;
}


namespace Moses
{

class BilingualLMState : public FFState
{
  int m_targetLen;
public:
  BilingualLMState(int targetLen)
    :m_targetLen(targetLen)
  {}

  int Compare(const FFState& other) const;
};

class BilingualLM : public StatefulFeatureFunction
{

private:
  void getSourceWords(const TargetPhrase &targetPhrase
                , int targetWordIdx
                , const Sentence &source_sent
                , const WordsRange &sourceWordRange
                , std::vector<int> &words
                , std::vector<std::string> &strings) const;

  void getTargetWords(Phrase &whole_phrase
                , int current_word_index
                , std::vector<int> &words
                , std::vector<std::string> &strings) const;

  size_t getState(Phrase &whole_phrase) const;

protected:
  // big data (vocab, weights, cache) shared among threads
  std::string m_filePath;
  nplm::neuralLM *m_neuralLM_shared;
  int m_nGramOrder;
  int target_ngrams;
  int source_ngrams;
  // thread-specific nplm for thread-safety
  mutable boost::thread_specific_ptr<nplm::neuralLM> m_neuralLM;

public:
  BilingualLM(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new BilingualLMState(0);
  }

  void Load();

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

};


}

