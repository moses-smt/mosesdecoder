#include "BleuScoreFeature.h"

#include "StaticData.h"

namespace Moses {

size_t BleuScoreState::bleu_order = 4;

BleuScoreState::BleuScoreState(): m_words(Output),
                                  m_ngram_counts(bleu_order),
                                  m_ngram_matches(bleu_order)
{
}

int BleuScoreState::Compare(const FFState& o) const
{
    if (&o == this)
        return 0;

    const BleuScoreState& other = dynamic_cast<const BleuScoreState&>(o);

    if (m_target_length < other.m_target_length)
        return -1;
    if (m_target_length > other.m_target_length)
        return 1;

    if (m_reference_length < other.m_reference_length)
        return -1;
    if (m_reference_length > other.m_reference_length)
        return 1;

    int c = m_words.Compare(other.m_words);

    if (c != 0)
        return c;

    for(size_t i = 0; i < m_ngram_counts.size(); i++) {
        if (m_ngram_counts[i] < other.m_ngram_counts[i])
            return -1;
        if (m_ngram_counts[i] > other.m_ngram_counts[i])
            return 1;
        if (m_ngram_matches[i] < other.m_ngram_matches[i])
            return -1;
        if (m_ngram_matches[i] > other.m_ngram_matches[i])
            return 1;
    }

    return 0;
}

BleuScoreFeature::BleuScoreFeature() {
    const_cast<ScoreIndexManager&>
        (StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);

}

FFState* BleuScoreFeature::Evaluate(const Hypothesis& cur_hypo, 
                                    const FFState* prev_state, 
                                    ScoreComponentCollection* accumulator) const {
    accumulator->PlusEquals( this, -1.0 );
    return NULL;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType &input) const
{
    return new BleuScoreState();
}


} // namespace.

