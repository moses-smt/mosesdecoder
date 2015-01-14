#pragma once

#include <string>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include <boost/thread/tss.hpp>
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
  std::vector<int> word_alignments; //Carry the word alignments. For hierarchical
  std::vector<int> neuralLM_ids; //Carry the neuralLMids of the previous target phrase to avoid calling GetWholePhrase. Hiero only.
public:
  BilingualLMState(size_t hash)
    :m_hash(hash) {
  }
  BilingualLMState(size_t hash, std::vector<int>& word_alignments_vec, std::vector<int>& neural_ids)
    :m_hash(hash)
    , word_alignments(word_alignments_vec)
    , neuralLM_ids(neural_ids) {
  }

  const std::vector<int>& GetWordAlignmentVector() const {
    return word_alignments;
  }

  const std::vector<int>& GetWordIdsVector() const {
    return neuralLM_ids;
  }

  int Compare(const FFState& other) const;
};

class BilingualLM : public StatefulFeatureFunction
{
private:
  virtual float Score(std::vector<int>& source_words, std::vector<int>& target_words) const = 0;

  virtual int getNeuralLMId(const Word& word, bool is_source_word) const = 0;

  virtual void loadModel() = 0;

  virtual const Word& getNullWord() const = 0;

  size_t selectMiddleAlignment(const std::set<size_t>& alignment_links) const;

  void getSourceWords(
    const TargetPhrase &targetPhrase,
    int targetWordIdx,
    const Sentence &source_sent,
    const WordsRange &sourceWordRange,
    std::vector<int> &words) const;

  void appendSourceWordsToVector(const Sentence &source_sent, std::vector<int> &words, int source_word_mid_idx) const;

  void getTargetWords(
    const Hypothesis &cur_hypo,
    const TargetPhrase &targetPhrase,
    int current_word_index,
    std::vector<int> &words) const;

  size_t getState(const Hypothesis &cur_hypo) const;

  void requestPrevTargetNgrams(const Hypothesis &cur_hypo, int amount, std::vector<int> &words) const;

  //Chart decoder
  void getTargetWordsChart(
    std::vector<int>& neuralLMids,
    int current_word_index,
    std::vector<int>& words,
    bool sentence_begin) const;

  size_t getStateChart(std::vector<int>& neuralLMids) const;

  //Get a vector of all target words IDs in the beginning of calculating NeuralLMids for the current phrase.
  void getAllTargetIdsChart(const ChartHypothesis& cur_hypo, size_t featureID, std::vector<int>& wordIds) const;
  //Get a vector of all alignments (mid_idx word)
  void getAllAlignments(const ChartHypothesis& cur_hypo, size_t featureID, std::vector<int>& alignemnts) const;

protected:
  // big data (vocab, weights, cache) shared among threads
  std::string m_filePath;
  int target_ngrams;
  int source_ngrams;

  //NeuralLM lookup
  FactorType word_factortype;
  FactorType pos_factortype;
  const Factor* BOS_factor;
  const Factor* EOS_factor;
  mutable Word BOS_word;
  mutable Word EOS_word;

public:
  BilingualLM(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new BilingualLMState(0);
  }

  void Load();

  void EvaluateInIsolation(
    const Phrase &source,
    const TargetPhrase &targetPhrase,
    ScoreComponentCollection &scoreBreakdown,
    ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWithSourceContext(
    const InputType &input,
    const InputPath &inputPath,
    const TargetPhrase &targetPhrase,
    const StackVec *stackVec,
    ScoreComponentCollection &scoreBreakdown,
    ScoreComponentCollection *estimatedFutureScore = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {};

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

