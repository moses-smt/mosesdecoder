#pragma once

#include <string>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include <boost/thread/tss.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/InputPath.h"
#include "moses/Manager.h"
#include "moses/FactorCollection.h"

namespace nplm {
  class neuralLM;
}


namespace Moses
{

class BilingualLMState : public FFState
{
  size_t m_hash;
  int first_word_index; //Doesn't matter for phrase based.
public:
  BilingualLMState(size_t hash)
    :m_hash(hash)
    ,first_word_index(0)
  {}
  BilingualLMState(size_t hash, int word_index)
    :m_hash(hash), first_word_index(word_index)
  {}

  int GetFirstWordIdx() const {
    return first_word_index;
  }

  int Compare(const FFState& other) const;
};

class BilingualLM : public StatefulFeatureFunction
{

private:
  void getSourceWords(const TargetPhrase &targetPhrase
                , int targetWordIdx
                , const Sentence &source_sent
                , const WordsRange &sourceWordRange
                , std::vector<int> &words) const;

  void getTargetWords(const Hypothesis &cur_hypo
                , const TargetPhrase &targetPhrase
                , int current_word_index
                , std::vector<int> &words) const;

  //size_t getState(const TargetPhrase &targetPhrase, std::vector<int> &prev_words) const;

  size_t getState(const Hypothesis &cur_hypo) const;

  void requestPrevTargetNgrams(const Hypothesis &cur_hypo, int amount, std::vector<int> &words) const;

  //Chart decoder
  void getTargetWordsChart(Phrase& whole_phrase
                , int current_word_index
                , std::vector<int> &words) const;

  size_t getStateChart(Phrase& whole_phrase) const;

  int getNeuralLMId(const Word& word) const;

  mutable std::map<const Factor*, int> neuralLMids;
  mutable boost::shared_mutex neuralLMids_lock;

protected:
  // big data (vocab, weights, cache) shared among threads
  std::string m_filePath;
  nplm::neuralLM *m_neuralLM_shared;
  int m_nGramOrder;
  int target_ngrams;
  int source_ngrams;
  bool premultiply;
  bool factored;
  int neuralLM_cache;
  int unknown_word_id;

  //NeuralLM lookup
  FactorType word_factortype;
  FactorType pos_factortype;
  const Factor* BOS_factor;
  const Factor* EOS_factor;
  mutable Word BOS_word_actual;
  mutable Word EOS_word_actual;
  const Word& BOS_word;
  const Word& EOS_word;

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
    const ChartHypothesis& cur_hypo ,
    int featureID, /* - used to index the state in the previous hypotheses */
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

};


}

