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
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"

namespace Moses
{

class BilingualLMState : public FFState
{
  size_t m_hash;
  int source_last_word_index; //Doesn't matter for phrase based. The last word source word of the previous hypothesis
  std::vector<int> word_alignments; //Carry the word alignments. For hierarchical
public:
  BilingualLMState(size_t hash)
    :m_hash(hash)
    , source_last_word_index(0)
  {}
  BilingualLMState(size_t hash, int source_word_index, std::vector<int>& word_alignments_vec)
    :m_hash(hash)
    , source_last_word_index(source_word_index)
    , word_alignments(word_alignments_vec)
  {}

  int GetLastSourceWordIdx() const {
    return source_last_word_index;
  }

  const std::vector<int>& GetWordAlignmentVector() const {
    return word_alignments;
  }

  int Compare(const FFState& other) const;
};

class BilingualLM : public StatefulFeatureFunction 
{

private:

  virtual float Score(std::vector<int>& source_words, std::vector<int>& target_words) const = 0;
  virtual int LookUpNeuralLMWord(const std::string str) const = 0;
  virtual void initSharedPointer() const = 0;
  virtual void loadModel() const = 0;

  void getSourceWords(const TargetPhrase &targetPhrase
                , int targetWordIdx
                , const Sentence &source_sent
                , const WordsRange &sourceWordRange
                , std::vector<int> &words) const;

  void appendSourceWordsToVector(const Sentence &source_sent, std::vector<int> &words, int source_word_mid_idx) const;

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

  //Returns the index of the source_word that the current target word uses
  int getSourceWordsChart(const TargetPhrase &targetPhrase
                , const ChartHypothesis& curr_hypothesis
                , int targetWordIdx
                , const Sentence &source_sent
                , size_t souce_phrase_start_pos
                , int next_nonterminal_index
                , int featureID
                , std::vector<int> &words) const;

  size_t getStateChart(Phrase& whole_phrase) const;

  int getNeuralLMId(const Word& word) const;

  mutable std::map<const Factor*, int> neuralLMids;
  mutable boost::shared_mutex neuralLMids_lock;

protected:
  // big data (vocab, weights, cache) shared among threads
  std::string m_filePath;
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

