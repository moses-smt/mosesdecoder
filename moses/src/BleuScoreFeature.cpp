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
                                    ScoreComponentCollection* accumulator) const
{
    std::map< Phrase, size_t > reference_ngrams;
    std::map< Phrase, size_t >::iterator reference_ngrams_iter;
    const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);

    BleuScoreState* new_state = new BleuScoreState(ps);

    float old_bleu, new_bleu;
    size_t num_new_words, ctx_start_idx, ctx_end_idx, ngram_start_idx,
           ngram_end_idx;

    // Calculate old bleu;
    old_bleu = CalculateBleu();

    // Get context and append new words.
    num_new_words = cur_hypo.GetTargetPhrase().GetSize();
    Phrase new_words = ps.m_words;
    new_words.Append(cur_hypo.GetTargetPhrase());

    for (size_t i = 0; i < num_new_words; i++) {
        for (size_t n = 0; n < BleuScoreState::bleu_order; n++) {
            ngram_end_idx = new_words.GetSize() - i;
            ngram_start_idx = ngram_end_idx - n;
            Phrase ngram = new_words.GetSubString(WordsRange(ngram_start_idx,
                                                             ngram_end_idx));

            // Update count of ngram order.
            new_state->m_ngram_counts[n]++;

            // Find out if this ngram is in the reference.
            reference_ngrams_iter = reference_ngrams.find(ngram);
            if (reference_ngrams_iter != reference_ngrams.end())
                new_state->m_ngram_matches[n]++;
        }
    }

    // Calculate new bleu.
    new_bleu = CalculateBleu();

    // Update state.
    ctx_end_idx = new_words.GetSize();
    ctx_start_idx = ctx_end_idx - (BleuScoreState::bleu_order - 1);
    new_state->m_words = new_words.GetSubString(WordsRange(ctx_start_idx,
                                                           ctx_start_idx));

    accumulator->PlusEquals( this, -1.0 );
    return new_state;
}

float BleuScoreFeature::CalculateBleu() const {
    return 0.0;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
    return new BleuScoreState();
}

} // namespace.

