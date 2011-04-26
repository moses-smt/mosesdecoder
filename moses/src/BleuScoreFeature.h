#ifndef BLUESCOREFEATURE_H
#define BLUESCOREFEATURE_H

#include <map>
#include <utility>
#include <string>
#include <vector>

#include "FeatureFunction.h"

#include "FFState.h"
#include "Phrase.h"

namespace Moses {

class BleuScoreFeature;

class BleuScoreState : public FFState {
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

    size_t m_source_phrase_length; // todo: delete

    // scaled reference length is needed for scoring incomplete hypotheses against reference translation
    float m_scaled_ref_length;

    std::vector< size_t > m_ngram_counts;
    std::vector< size_t > m_ngram_matches;
};

std::ostream& operator<<(std::ostream& out, const BleuScoreState& state);

typedef std::map< Phrase, size_t > NGrams;

class BleuScoreFeature : public StatefulFeatureFunction {
public:
	BleuScoreFeature();
    BleuScoreFeature(bool useScaledReference, bool scaleByInputLength, float BPfactor, float historySmoothing);

    std::string GetScoreProducerDescription() const
    {
    	return "BleuScoreFeature";
    }

    std::string GetScoreProducerWeightShortName() const
    {
        return "bl";
    }

    size_t GetNumScoreComponents() const
    {
        return 1;
    }

    void PrintHistory(std::ostream& out) const;
    void LoadReferences(const std::vector< std::vector< std::string > > &);
    void SetCurrentSourceLength(size_t);
    void SetCurrentReference(size_t);
    void SetBPfactor(float);
    void UpdateHistory(const std::vector< const Word* >&);
    void UpdateHistory(const std::vector< std::vector< const Word* > >& hypos, std::vector<size_t>& sourceLengths, std::vector<size_t>& ref_ids, size_t rank, size_t epoch);
    void GetNgramMatchCounts(Phrase&,
                             const NGrams&,
                             std::vector< size_t >&,
                             std::vector< size_t >&,
                             size_t skip = 0) const;
    void GetClippedNgramMatchesAndCounts(Phrase&,
                                 const NGrams&,
                                 std::vector< size_t >&,
                                 std::vector< size_t >&,
                                 size_t skip = 0) const;

    FFState* Evaluate( const Hypothesis& cur_hypo, 
                       const FFState* prev_state, 
                       ScoreComponentCollection* accumulator) const;
    float CalculateBleu(BleuScoreState*) const;
    std::vector<float> CalculateBleuOfCorpus(const std::vector< std::vector< const Word* > >& hypos, const std::vector<size_t>& ref_ids);
    const FFState* EmptyHypothesisState(const InputType&) const;

private:
    size_t m_cur_source_length;
    std::map< size_t, std::pair< size_t, NGrams > > m_refs;
    NGrams m_cur_ref_ngrams;
    size_t m_cur_ref_length;

    // whether or not to use the scaled reference
    bool m_use_scaled_reference;

    // whether or not to scale the BLEU score by a history of the input size
    bool m_scale_by_input_length;

    // increase penalty for short translations
    float m_BP_factor;

    float m_historySmoothing;

    // counts for pseudo-document big_O
    std::vector< float > m_count_history;
    std::vector< float > m_match_history;
    float m_source_length_history;
    float m_target_length_history;
    float m_ref_length_history;
};

} // Namespace.

#endif //BLUESCOREFEATURE_H

