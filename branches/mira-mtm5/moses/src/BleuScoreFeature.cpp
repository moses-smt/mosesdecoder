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

BleuScoreFeature::BleuScoreFeature() {
    const_cast<ScoreIndexManager&>
        (StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);

}

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

FFState* BleuScoreFeature::Evaluate(const Hypothesis& cur_hypo, 
                                    const FFState* prev_state, 
                                    ScoreComponentCollection* accumulator) const
{
    std::map< Phrase, size_t >::const_iterator reference_ngrams_iter;
    const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);
    BleuScoreState* new_state = new BleuScoreState(ps);
    //cerr << "PS: " << ps << endl;

    float old_bleu, new_bleu;
    size_t num_new_words, ctx_start_idx, ctx_end_idx, ngram_start_idx,
           ngram_end_idx;

    // Calculate old bleu;
    old_bleu = CalculateBleu(new_state);

    // Get context and append new words.
    num_new_words = cur_hypo.GetTargetPhrase().GetSize();
    Phrase new_words = ps.m_words;
    new_words.Append(cur_hypo.GetTargetPhrase());
    //cerr << "NW: " << new_words << endl;

    for (size_t i = 0; i < num_new_words; i++) {
        for (size_t n = 1; n <= BleuScoreState::bleu_order; n++) {
            // Break if we don't have enough context to do ngrams of order n.
            if (new_words.GetSize() - i < n) break;

            ngram_end_idx = new_words.GetSize() - i - 1;
            ngram_start_idx = ngram_end_idx - (n-1);

            Phrase ngram = new_words.GetSubString(WordsRange(ngram_start_idx,
                                                             ngram_end_idx));

            // Update count of ngram order.
            new_state->m_ngram_counts[n-1]++;

            // Find out if this ngram is in the reference.
            reference_ngrams_iter = m_cur_ref_ngrams.find(ngram);
            //cerr << "Searching for " << ngram << " ... ";
            if (reference_ngrams_iter != m_cur_ref_ngrams.end()) {
                new_state->m_ngram_matches[n-1]++;
                //cerr << "FOUND" << endl;
            } else {
              //cerr << "NOT FOUND" << endl;
            }
        }
    }

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

    // Calculate ngram precisions.
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++)
        if (state->m_ngram_counts[i]) {
          precision *= (float)(state->m_ngram_matches[i]) / state->m_ngram_counts[i];
        }

    // Apply brevity penalty if applicable.
    if (state->m_source_length < state->m_scaled_ref_length)
        precision *= exp(1 - state->m_scaled_ref_length /
                         state->m_source_length);

    return precision;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
    return new BleuScoreState();
}

} // namespace.

