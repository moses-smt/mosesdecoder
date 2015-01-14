#ifndef BLUESCOREFEATURE_H
#define BLUESCOREFEATURE_H

#include <utility>
#include <string>
#include <vector>

#include <boost/unordered_map.hpp>

#include "StatefulFeatureFunction.h"

#include "moses/FF/FFState.h"
#include "moses/Phrase.h"
#include "moses/ChartHypothesis.h"

namespace Moses
{

class BleuScoreFeature;

class BleuScoreState : public FFState
{
public:
  friend class BleuScoreFeature;
  static size_t bleu_order;

  BleuScoreState();
  virtual int Compare(const FFState& other) const;
  void print(std::ostream& out) const;

private:
  Phrase m_words;
  size_t m_source_length;
  size_t m_target_length;

  // scaled reference length is needed for scoring incomplete hypotheses against reference translation
  float m_scaled_ref_length;

  std::vector< size_t > m_ngram_counts;
  std::vector< size_t > m_ngram_matches;

  void AddNgramCountAndMatches(std::vector< size_t >& counts, std::vector< size_t >& matches);
};


std::ostream& operator<<(std::ostream& out, const BleuScoreState& state);

typedef boost::unordered_map< Phrase, size_t > NGrams;

class RefValue : public  std::pair<std::vector<size_t>,NGrams>
{
public:
  RefValue& operator=( const RefValue& rhs ) {
    first = rhs.first;
    second = rhs.second;
    return *this;
  }
};


class BleuScoreFeature : public StatefulFeatureFunction
{
public:
  static const std::vector<BleuScoreFeature*>& GetColl() {
    return s_staticColl;
  }

  typedef boost::unordered_map<size_t, RefValue > RefCounts;
  typedef boost::unordered_map<size_t, NGrams> Matches;

  BleuScoreFeature(const std::string &line);

  void SetParameter(const std::string& key, const std::string& value);

  std::vector<float> DefaultWeights() const;

  void PrintHistory(std::ostream& out) const;
  void LoadReferences(const std::vector< std::vector< std::string > > &);
  void SetCurrSourceLength(size_t);
  void SetCurrNormSourceLength(size_t);
  void SetCurrShortestRefLength(size_t);
  void SetCurrAvgRefLength(size_t sent_id);
  void SetAvgInputLength (float l) {
    m_avg_input_length = l;
  }
  void SetCurrReferenceNgrams(size_t sent_id);
  size_t GetShortestRefIndex(size_t ref_id);
  size_t GetClosestRefLength(size_t ref_id, int hypoLength);
  void UpdateHistory(const std::vector< const Word* >&);
  void UpdateHistory(const std::vector< std::vector< const Word* > >& hypos, std::vector<size_t>& sourceLengths, std::vector<size_t>& ref_ids, size_t rank, size_t epoch);
  void PrintRefLength(const std::vector<size_t>& ref_ids);
  void SetBleuParameters(bool disable, bool sentenceBleu, bool scaleByInputLength, bool scaleByAvgInputLength,
                         bool scaleByInverseLength, bool scaleByAvgInverseLength,
                         float scaleByX, float historySmoothing, size_t scheme, bool simpleHistoryBleu);

  void GetNgramMatchCounts(Phrase&,
                           const NGrams&,
                           std::vector< size_t >&,
                           std::vector< size_t >&,
                           size_t skip = 0) const;
  void GetNgramMatchCounts_prefix(Phrase&,
                                  const NGrams&,
                                  std::vector< size_t >&,
                                  std::vector< size_t >&,
                                  size_t new_start_indices,
                                  size_t last_end_index) const;
  void GetNgramMatchCounts_overlap(Phrase& phrase,
                                   const NGrams& ref_ngram_counts,
                                   std::vector< size_t >& ret_counts,
                                   std::vector< size_t >& ret_matches,
                                   size_t overlap_index) const;
  void GetClippedNgramMatchesAndCounts(Phrase&,
                                       const NGrams&,
                                       std::vector< size_t >&,
                                       std::vector< size_t >&,
                                       size_t skip = 0) const;

  FFState* EvaluateWhenApplied( const Hypothesis& cur_hypo,
                                const FFState* prev_state,
                                ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(const ChartHypothesis& cur_hypo,
                               int featureID,
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
  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
  }

  bool Enabled() const {
    return m_enabled;
  }

  bool IsUseable(const FactorMask &mask) const;

  float CalculateBleu(BleuScoreState*) const;
  float CalculateBleu(Phrase translation) const;
  const FFState* EmptyHypothesisState(const InputType&) const;

  float GetSourceLengthHistory() {
    return m_source_length_history;
  }
  float GetTargetLengthHistory() {
    return m_target_length_history;
  }
  float GetAverageInputLength() {
    return m_avg_input_length;
  }

private:
  static std::vector<BleuScoreFeature*> s_staticColl;

  bool m_enabled;
  bool m_sentence_bleu;
  bool m_simple_history_bleu;

  // counts for pseudo-document
  std::vector< float > m_count_history;
  std::vector< float > m_match_history;
  float m_source_length_history;
  float m_target_length_history;
  float m_ref_length_history;

  size_t m_cur_source_length;
  size_t m_cur_norm_source_length; // length without <s>, </s>
  RefCounts m_refs;
  NGrams m_cur_ref_ngrams;
  float m_cur_ref_length;

  // scale BLEU score by history of input length
  bool m_scale_by_input_length;
  bool m_scale_by_avg_input_length;

  // scale by the inverse of the input length * 100
  bool m_scale_by_inverse_length;
  bool m_scale_by_avg_inverse_length;

  float m_avg_input_length;

  float m_scale_by_x;

  // smoothing factor for history counts
  float m_historySmoothing;

  enum SmoothingScheme { PLUS_ONE = 1, PLUS_POINT_ONE = 2, PAPINENI = 3 };
  SmoothingScheme m_smoothing_scheme;
};

} // Namespace.

#endif //BLUESCOREFEATURE_H

