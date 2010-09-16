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

    if (m_ref_length < other.m_ref_length)
        return -1;
    if (m_ref_length > other.m_ref_length)
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

void BleuScoreFeature::LoadReferences(const std::vector<
                                      std::vector< std::string > > &refs)
{
    FactorCollection& fc = FactorCollection::Instance();
    for (size_t ref_id = 0; ref_id < refs.size(); ref_id++) {
        const std::vector< std::string >& ref = refs[ref_id];
        std::pair< size_t, NGrams > ref_pair;
        ref_pair.first = ref.size();
        for (size_t order = 1; order < BleuScoreState::bleu_order; order++) {
            for (size_t end_idx = order; end_idx < ref.size(); end_idx++) {
                Phrase ngram(Output);
                for (size_t s_idx = end_idx - order; s_idx < end_idx; s_idx++) {
                    const Factor* f = fc.AddFactor(Output, 0, ref[s_idx]);
                    Word w;
                    w.SetFactor(0, f);
                    ngram.AddWord(w);
                }
                ref_pair.second[ngram] += 1;
            }
        }
        m_refs[ref_id] = ref_pair;
    }
}

void BleuScoreFeature::SetCurrentReference(size_t ref_id) {
    m_cur_ref_length = m_refs[ref_id].first;
    m_cur_ref_ngrams = m_refs[ref_id].second;
}

FFState* BleuScoreFeature::Evaluate(const Hypothesis& cur_hypo, 
                                    const FFState* prev_state, 
                                    ScoreComponentCollection* accumulator) const
{
    NGrams::const_iterator reference_ngrams_iter;
    const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);
    BleuScoreState* new_state = new BleuScoreState(ps);

    float old_bleu, new_bleu;
    size_t num_new_words, ctx_start_idx, ctx_end_idx, ngram_start_idx,
           ngram_end_idx;

    // Calculate old bleu;
    old_bleu = CalculateBleu(new_state);

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
            reference_ngrams_iter = m_cur_ref_ngrams.find(ngram);
            if (reference_ngrams_iter != m_cur_ref_ngrams.end())
                new_state->m_ngram_matches[n]++;
        }
    }

    // Update state.
    ctx_end_idx = new_words.GetSize();
    ctx_start_idx = ctx_end_idx - (BleuScoreState::bleu_order - 1);
    new_state->m_words = new_words.GetSubString(WordsRange(ctx_start_idx,
                                                           ctx_start_idx));
    new_state->m_target_length = cur_hypo.GetSize();
    new_state->m_ref_length = cur_hypo.GetWordsBitmap().GetNumWordsCovered() /
                              m_cur_ref_length;

    // Calculate new bleu.
    new_bleu = CalculateBleu(new_state);

    // Set score to difference in bleu scores.
    accumulator->PlusEquals(this, new_bleu - old_bleu);

    return new_state;
}

float BleuScoreFeature::CalculateBleu(BleuScoreState* state) const {
    float precision = 1.0;

    // Calculate ngram precisions.
    for (size_t i = 1; i < BleuScoreState::bleu_order; i++)
        precision *= state->m_ngram_matches[i] / state->m_ngram_counts[i];

    // Apply brevity penalty if applicable.
    if (state->m_target_length < state->m_ref_length)
        precision *= exp(1 - state->m_ref_length / state->m_target_length);

    return precision;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
    return new BleuScoreState();
}

} // namespace.

