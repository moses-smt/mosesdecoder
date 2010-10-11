#include "BleuScoreFeature.h"

#include "StaticData.h"

using namespace std;

namespace Moses {

size_t BleuScoreState::bleu_order = 4;

BleuScoreState::BleuScoreState(): m_words(Output),
                                  m_source_length(0),
                                  m_scaled_ref_length(0),
                                  m_ngram_counts(bleu_order),
                                  m_ngram_matches(bleu_order)
{
}

int BleuScoreState::Compare(const FFState& o) const
{
    if (&o == this)
        return 0;

    const BleuScoreState& other = dynamic_cast<const BleuScoreState&>(o);

    if (m_source_length < other.m_source_length)
        return -1;
    if (m_source_length > other.m_source_length)
        return 1;

    if (m_scaled_ref_length < other.m_scaled_ref_length)
        return -1;
    if (m_scaled_ref_length > other.m_scaled_ref_length)
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
std::ostream& operator<<(std::ostream& out, const BleuScoreState& state) {
  state.print(out);
  return out;
}

void BleuScoreState::print(std::ostream& out) const {
  out << "ref=" << m_scaled_ref_length << 
    ";source=" << m_source_length << ";counts=";
  for (size_t i = 0; i < bleu_order; ++i) {
    out << m_ngram_matches[i] << "/" << m_ngram_counts[i] << ",";
  }
  out << "ctxt=" << m_words;
    
}

BleuScoreFeature::BleuScoreFeature():
                                 StatefulFeatureFunction("BleuScore"),      
                                 m_count_history(BleuScoreState::bleu_order),
                                 m_match_history(BleuScoreState::bleu_order),
                                 m_target_length_history(0),
                                 m_ref_length_history(0) {}

void BleuScoreFeature::LoadReferences(const std::vector< std::vector< std::string > >& refs)
{
    FactorCollection& fc = FactorCollection::Instance();
    for (size_t file_id = 0; file_id < refs.size(); file_id++) {
      for (size_t ref_id = 0; ref_id < refs[file_id].size(); ref_id++) {
          const string& ref = refs[file_id][ref_id];
          vector<string> refTokens  = Tokenize(ref);
          std::pair< size_t, std::map< Phrase, size_t > > ref_pair;
          ref_pair.first = refTokens.size();
          for (size_t order = 1; order <= BleuScoreState::bleu_order; order++) {
              for (size_t end_idx = order; end_idx <= refTokens.size(); end_idx++) {
                  Phrase ngram(Output);
                  //cerr << "start: " << end_idx-order << " end: " << end_idx << endl;
                  for (size_t s_idx = end_idx - order; s_idx < end_idx; s_idx++) {
                      const Factor* f = fc.AddFactor(Output, 0, refTokens[s_idx]);
                      Word w;
                      w.SetFactor(0, f);
                      ngram.AddWord(w);
                  }
                  //cerr << "Ref: " << ngram << endl;
                  ref_pair.second[ngram] += 1;
              }
          }
          m_refs[ref_id] = ref_pair;
      }
    }
}

void BleuScoreFeature::SetCurrentReference(size_t ref_id) {
    m_cur_ref_length = m_refs[ref_id].first;
    m_cur_ref_ngrams = m_refs[ref_id].second;
}

void BleuScoreFeature::UpdateHistory(const vector< const Word* >& hypo) {
    Phrase phrase(Output, hypo);
    std::vector< size_t > ngram_counts(BleuScoreState::bleu_order);
    std::vector< size_t > ngram_matches(BleuScoreState::bleu_order);
    GetNgramMatchCounts(phrase, m_cur_ref_ngrams, ngram_counts, ngram_matches, 0);
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
        m_count_history[i] = 0.9 * (m_count_history[i] + ngram_counts[i]);
        m_match_history[i] = 0.9 * (m_match_history[i] + ngram_matches[i]);
    }
    m_target_length_history = 0.9 * (m_target_length_history + hypo.size());
    m_ref_length_history = 0.9 * (m_ref_length_history + m_cur_ref_length);
}

void BleuScoreFeature::GetNgramMatchCounts(Phrase& phrase,
                                           const NGrams& ref_ngram_counts,
                                           std::vector< size_t >& ret_counts,
                                           std::vector< size_t >& ret_matches,
                                           size_t skip_first) const
{
    std::map< Phrase, size_t >::const_iterator ref_ngram_counts_iter;
    size_t ngram_start_idx, ngram_end_idx;

    for (size_t end_idx = skip_first; end_idx < phrase.GetSize(); end_idx++) {
        for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
            if (order > end_idx) break;

            ngram_end_idx = end_idx;
            ngram_start_idx = end_idx - order;

            Phrase ngram = phrase.GetSubString(WordsRange(ngram_start_idx,
                                                          ngram_end_idx));

            ret_counts[order]++;

            ref_ngram_counts_iter = ref_ngram_counts.find(ngram);
            if (ref_ngram_counts_iter != ref_ngram_counts.end())
                ret_matches[order]++;
        }
    }
}

FFState* BleuScoreFeature::Evaluate(const Hypothesis& cur_hypo, 
                                    const FFState* prev_state, 
                                    ScoreComponentCollection* accumulator) const
{
    std::map< Phrase, size_t >::const_iterator reference_ngrams_iter;
    const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);
    BleuScoreState* new_state = new BleuScoreState(ps);
    //cerr << "PS: " << ps << endl;

    float old_bleu, new_bleu;
    size_t num_new_words, ctx_start_idx, ctx_end_idx;

    // Calculate old bleu;
    old_bleu = CalculateBleu(new_state);

    // Get context and append new words.
    num_new_words = cur_hypo.GetTargetPhrase().GetSize();
    Phrase new_words = ps.m_words;
    new_words.Append(cur_hypo.GetTargetPhrase());
    //cerr << "NW: " << new_words << endl;

    GetNgramMatchCounts(new_words,
                        m_cur_ref_ngrams,
                        new_state->m_ngram_counts,
                        new_state->m_ngram_matches,
                        new_state->m_words.GetSize());

    // Update state.
    ctx_end_idx = new_words.GetSize()-1;
    size_t bleu_context_length = BleuScoreState::bleu_order -1;
    if (ctx_end_idx > bleu_context_length) {
      ctx_start_idx = ctx_end_idx - bleu_context_length;
    } else {
      ctx_start_idx = 0;
    }
    new_state->m_words = new_words.GetSubString(WordsRange(ctx_start_idx,
                                                           ctx_end_idx));

    new_state->m_source_length += cur_hypo.GetSourcePhrase()->GetSize();
    WordsBitmap coverageVector = cur_hypo.GetWordsBitmap();
    new_state->m_scaled_ref_length = m_cur_ref_length * 
        ((float)coverageVector.GetNumWordsCovered() / coverageVector.GetSize());
      

    // Calculate new bleu.
    new_bleu = CalculateBleu(new_state);
    //cerr << "NS: " << *new_state << " NB " << new_bleu << endl;

    // Set score to difference in bleu scores.
    accumulator->PlusEquals(this, new_bleu - old_bleu);

    return new_state;
}

float BleuScoreFeature::CalculateBleu(BleuScoreState* state) const {
    if (!state->m_ngram_counts[0]) return 0;
    float precision = 1.0;
    float smoothed_count, smoothed_matches;
    float smoothed_ref_length = m_ref_length_history +
                                state->m_scaled_ref_length;
    float smoothed_target_length = m_target_length_history + 
                                   state->m_source_length;

    // Calculate ngram precisions.
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++)
        if (state->m_ngram_counts[i]) {
            smoothed_matches = m_match_history[i] + state->m_ngram_matches[i];
            smoothed_count = m_count_history[i] + state->m_ngram_counts[i];
            precision *= smoothed_matches / smoothed_count;
        }

    // Apply brevity penalty if applicable.
    if (state->m_source_length < state->m_scaled_ref_length) {
        precision *= exp(1 - (smoothed_ref_length / smoothed_target_length));
    }

    return precision;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
    return new BleuScoreState();
}

} // namespace.

