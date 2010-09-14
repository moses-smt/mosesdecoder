#ifndef BLUESCOREFEATURE_H
#define BLUESCOREFEATURE_H

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

private:
    Phrase m_words;

    size_t m_target_length;
    float m_reference_length;
    std::vector< size_t > m_ngram_counts;
    std::vector< size_t > m_ngram_matches;
};

class BleuScoreFeature : public StatefulFeatureFunction {
public:
    BleuScoreFeature();

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


    FFState* Evaluate( const Hypothesis& cur_hypo, 
                       const FFState* prev_state, 
                       ScoreComponentCollection* accumulator) const;
    const FFState* EmptyHypothesisState() const;
};

} // Namespace.

#endif //BLUESCOREFEATURE_H
