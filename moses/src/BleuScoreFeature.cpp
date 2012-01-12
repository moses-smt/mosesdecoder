#include "BleuScoreFeature.h"

#include "StaticData.h"

using namespace std;

namespace Moses {

size_t BleuScoreState::bleu_order = 4;

BleuScoreState::BleuScoreState(): m_words(Output,1),
//                                  m_source_length(0),
                                  m_target_length(0),
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

//    if (m_source_length < other.m_source_length)
//    	return -1;
//    if (m_source_length > other.m_source_length)
//    	return 1;

    if (m_target_length < other.m_target_length)
        return -1;
    if (m_target_length > other.m_target_length)
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
  out << "ref=" << m_scaled_ref_length //<< ";source=" << m_source_length
	  << ";target=" << m_target_length << ";counts=";
  for (size_t i = 0; i < bleu_order; ++i) {
    out << m_ngram_matches[i] << "/" << m_ngram_counts[i] << ",";
  }
  out << "ctxt=" << m_words;
    
}

void BleuScoreFeature::PrintHistory(std::ostream& out) const {
	out << "source length history=" << m_source_length_history << endl;
	out << "target length history=" << m_target_length_history << endl;
	out << "ref length history=" << m_ref_length_history << endl;

  for (size_t i = 0; i < BleuScoreState::bleu_order; ++i) {
    out << "match history/count history (" << i << "):" << m_match_history[i] << "/" << m_count_history[i] << endl;
  }
}

void BleuScoreFeature::SetBleuParameters(bool scaleByInputLength, bool scaleByRefLength, bool scaleByAvgLength,
		bool scaleByTargetLengthLinear, bool scaleByTargetLengthTrend,
		float scaleByX, float historySmoothing, size_t scheme, float relaxBP) {
	m_scale_by_input_length = scaleByInputLength;
	m_scale_by_ref_length = scaleByRefLength;
	m_scale_by_target_length_linear = scaleByTargetLengthLinear;
	m_scale_by_target_length_trend = scaleByTargetLengthTrend;
	m_scale_by_avg_length = scaleByAvgLength;
	m_scale_by_x = scaleByX;
	m_historySmoothing = historySmoothing;
	m_smoothing_scheme = (SmoothingScheme)scheme;
	m_relax_BP = relaxBP;
}

void BleuScoreFeature::LoadReferences(const std::vector< std::vector< std::string > >& refs)
{
	m_refs.clear();
    FactorCollection& fc = FactorCollection::Instance();
    cerr << "Number of reference files: " << refs.size() << endl; 
    for (size_t file_id = 0; file_id < refs.size(); file_id++) {
      for (size_t ref_id = 0; ref_id < refs[file_id].size(); ref_id++) {
          const string& ref = refs[file_id][ref_id];
          vector<string> refTokens  = Tokenize(ref);
          m_refs[ref_id] = pair<size_t,NGrams>();
           pair<size_t,NGrams>& ref_pair = m_refs[ref_id];
          ref_pair.first = refTokens.size();
          for (size_t order = 1; order <= BleuScoreState::bleu_order; order++) {
              for (size_t end_idx = order; end_idx <= refTokens.size(); end_idx++) {
                  Phrase ngram(Output,1);
                  for (size_t s_idx = end_idx - order; s_idx < end_idx; s_idx++) {
                      const Factor* f = fc.AddFactor(Output, 0, refTokens[s_idx]);
                      Word w;
                      w.SetFactor(0, f);
                      ngram.AddWord(w);
                  }
                  ref_pair.second[ngram] += 1;
              }
          }
      }
    }
}

void BleuScoreFeature::SetCurrentSourceLength(size_t source_length) {
    m_cur_source_length = source_length;
}

void BleuScoreFeature::SetCurrentReference(size_t ref_id) {
    m_cur_ref_length = m_refs[ref_id].first;
    m_cur_ref_ngrams = m_refs[ref_id].second;
}

/*
 * Update the pseudo-document O after each translation of a source sentence.
 * (O is an exponentially-weighted moving average of vectors c(e;{r_k}))
 * O = m_historySmoothing * (O + c(e_oracle))
 * O_f = m_historySmoothing * (O_f + |f|)		input length of pseudo-document
 */
void BleuScoreFeature::UpdateHistory(const vector< const Word* >& hypo) {
    Phrase phrase(Output, hypo);
    std::vector< size_t > ngram_counts(BleuScoreState::bleu_order);
    std::vector< size_t > ngram_matches(BleuScoreState::bleu_order);

    // compute vector c(e;{r_k}):
    // vector of effective reference length, number of ngrams in e, number of ngram matches between e and r_k
    GetNgramMatchCounts(phrase, m_cur_ref_ngrams, ngram_counts, ngram_matches, 0);

    // update counts and matches for every ngram length with counts from hypo
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
        m_count_history[i] = m_historySmoothing * (m_count_history[i] + ngram_counts[i]);
        m_match_history[i] = m_historySmoothing * (m_match_history[i] + ngram_matches[i]);
    }

    // update counts for reference and target length
    m_source_length_history = m_historySmoothing * (m_source_length_history + m_cur_source_length);
    m_target_length_history = m_historySmoothing * (m_target_length_history + hypo.size());
    m_ref_length_history = m_historySmoothing * (m_ref_length_history + m_cur_ref_length);
}

/*
 * Update history with a batch of translations
 */
void BleuScoreFeature::UpdateHistory(const vector< vector< const Word* > >& hypos, vector<size_t>& sourceLengths, vector<size_t>& ref_ids, size_t rank, size_t epoch) {
	for (size_t batchPosition = 0; batchPosition < hypos.size(); ++batchPosition){
	    Phrase phrase(Output, hypos[batchPosition]);
	    std::vector< size_t > ngram_counts(BleuScoreState::bleu_order);
	    std::vector< size_t > ngram_matches(BleuScoreState::bleu_order);

	    // set current source and reference information for each oracle in the batch
	    size_t cur_source_length = sourceLengths[batchPosition];
	    size_t cur_ref_length = m_refs[ref_ids[batchPosition]].first;
	    NGrams cur_ref_ngrams = m_refs[ref_ids[batchPosition]].second;
	    cerr << "reference length: " << cur_ref_length << endl;

	    // compute vector c(e;{r_k}):
	    // vector of effective reference length, number of ngrams in e, number of ngram matches between e and r_k
	    GetNgramMatchCounts(phrase, cur_ref_ngrams, ngram_counts, ngram_matches, 0);

	    // update counts and matches for every ngram length with counts from hypo
	    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
	        m_count_history[i] += ngram_counts[i];
	        m_match_history[i] += ngram_matches[i];

	        // do this for last position in batch
	        if (batchPosition == hypos.size() - 1) {
	        	m_count_history[i] *= m_historySmoothing;
	        	m_match_history[i] *= m_historySmoothing;
	        }
	    }

	    // update counts for reference and target length
	    m_source_length_history += cur_source_length;
	    m_target_length_history += hypos[batchPosition].size();
	    m_ref_length_history += cur_ref_length;

	    // do this for last position in batch
	    if (batchPosition == hypos.size() - 1) {
	    	cerr << "Rank " << rank << ", epoch " << epoch << " ,source length history: " << m_source_length_history << " --> " << m_source_length_history * m_historySmoothing << endl;
	    	cerr << "Rank " << rank << ", epoch " << epoch << " ,target length history: " << m_target_length_history << " --> " << m_target_length_history * m_historySmoothing << endl;
	    	m_source_length_history *= m_historySmoothing;
	    	m_target_length_history *= m_historySmoothing;
	    	m_ref_length_history *= m_historySmoothing;
	    }
	}
}

/*
 * Print batch of reference translations
 */
void BleuScoreFeature::PrintReferenceLength(const vector<size_t>& ref_ids) {
	for (size_t batchPosition = 0; batchPosition < ref_ids.size(); ++batchPosition){
	    size_t cur_ref_length = m_refs[ref_ids[batchPosition]].first;
	    cerr << "reference length: " << cur_ref_length << endl;
	}
}

size_t BleuScoreFeature::GetReferenceLength(size_t ref_id) {
	size_t cur_ref_length = m_refs[ref_id].first;
	return cur_ref_length;
}

/*
 * Given a phrase (current translation) calculate its ngram counts and
 * its ngram matches against the ngrams in the reference translation
 */
void BleuScoreFeature::GetNgramMatchCounts(Phrase& phrase,
                                           const NGrams& ref_ngram_counts,
                                           std::vector< size_t >& ret_counts,
                                           std::vector< size_t >& ret_matches,
                                           size_t skip_first) const
{
    NGrams::const_iterator ref_ngram_counts_iter;
    size_t ngram_start_idx, ngram_end_idx;

    // Chiang et al (2008) use unclipped counts of ngram matches
    for (size_t end_idx = skip_first; end_idx < phrase.GetSize(); end_idx++) {
        for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
            if (order > end_idx) break;

            ngram_end_idx = end_idx;
            ngram_start_idx = end_idx - order;

            Phrase ngram = phrase.GetSubString(WordsRange(ngram_start_idx, ngram_end_idx), 0);
            ret_counts[order]++;

            ref_ngram_counts_iter = ref_ngram_counts.find(ngram);
            if (ref_ngram_counts_iter != ref_ngram_counts.end())
                ret_matches[order]++;
        }
    }
}

void BleuScoreFeature::GetClippedNgramMatchesAndCounts(Phrase& phrase,
                                           const NGrams& ref_ngram_counts,
                                           std::vector< size_t >& ret_counts,
                                           std::vector< size_t >& ret_matches,
                                           size_t skip_first) const
{
	NGrams::const_iterator ref_ngram_counts_iter;
	size_t ngram_start_idx, ngram_end_idx;

	Matches ngram_matches;
	for (size_t end_idx = skip_first; end_idx < phrase.GetSize(); end_idx++) {
		for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
			if (order > end_idx) break;

			ngram_end_idx = end_idx;
			ngram_start_idx = end_idx - order;

			Phrase ngram = phrase.GetSubString(WordsRange(ngram_start_idx, ngram_end_idx), 0);
			ret_counts[order]++;

			ref_ngram_counts_iter = ref_ngram_counts.find(ngram);
			if (ref_ngram_counts_iter != ref_ngram_counts.end()) {
				ngram_matches[order][ngram]++;
			}
		}
	}

	// clip ngram matches
	for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
		NGrams::const_iterator iter;

		// iterate over ngram counts for every ngram order
		for (iter=ngram_matches[order].begin(); iter != ngram_matches[order].end(); ++iter) {
			ref_ngram_counts_iter = ref_ngram_counts.find(iter->first);
			if (iter->second > ref_ngram_counts_iter->second) {
				ret_matches[order] += ref_ngram_counts_iter->second;
			}
			else {
				ret_matches[order] += iter->second;
			}
    }
	}
}

/*
 * Given a previous state, compute Bleu score for the updated state with an additional target
 * phrase translated.
 */
FFState* BleuScoreFeature::Evaluate(const Hypothesis& cur_hypo, 
                                    const FFState* prev_state, 
                                    ScoreComponentCollection* accumulator) const
{
    NGrams::const_iterator reference_ngrams_iter;
    const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);
    BleuScoreState* new_state = new BleuScoreState(ps);
    //cerr << "PS: " << ps << endl;

    float old_bleu, new_bleu;
    size_t num_new_words, ctx_start_idx, ctx_end_idx;

    // Calculate old bleu;
    old_bleu = CalculateBleu(new_state);

    // Get context and append new words.
    num_new_words = cur_hypo.GetCurrTargetLength();
    if (num_new_words == 0) {
	return new_state;
    }
 
    Phrase new_words = ps.m_words;
    new_words.Append(cur_hypo.GetCurrTargetPhrase());
    //cerr << "NW: " << new_words << endl;

    // get ngram matches for new words
    GetNgramMatchCounts(new_words,
                        m_cur_ref_ngrams,
                        new_state->m_ngram_counts,
                        new_state->m_ngram_matches,
                        new_state->m_words.GetSize());

    // Update state variables
    ctx_end_idx = new_words.GetSize()-1;
    size_t bleu_context_length = BleuScoreState::bleu_order -1;
    if (ctx_end_idx > bleu_context_length) {
      ctx_start_idx = ctx_end_idx - bleu_context_length;
    } else {
      ctx_start_idx = 0;
    }

    WordsBitmap coverageVector = cur_hypo.GetWordsBitmap();
    new_state->m_source_length = coverageVector.GetNumWordsCovered();

    new_state->m_words = new_words.GetSubString(WordsRange(ctx_start_idx,
                                                           ctx_end_idx));
    new_state->m_target_length += cur_hypo.GetCurrTargetLength();

    // we need a scaled reference length to compare the current target phrase to the corresponding reference phrase
    new_state->m_scaled_ref_length = m_cur_ref_length * 
        ((float)coverageVector.GetNumWordsCovered() / coverageVector.GetSize());

    // Calculate new bleu.
    new_bleu = CalculateBleu(new_state);

    // Set score to new Bleu score
    accumulator->PlusEquals(this, new_bleu - old_bleu);

    return new_state;
}

/*
 * Calculate Bleu score for a partial hypothesis given as state.
 */
float BleuScoreFeature::CalculateBleu(BleuScoreState* state) const {
  if (!state->m_ngram_counts[0]) return 0;
  if (!state->m_ngram_matches[0]) return 0;      	// if we have no unigram matches, score should be 0

  float precision = 1.0;
  float smooth = 1;
  float smoothed_count, smoothed_matches;

  // Calculate geometric mean of modified ngram precisions
  // BLEU = BP * exp(SUM_1_4 1/4 * log p_n)
  // 		= BP * 4th root(PRODUCT_1_4 p_n)
  for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
    if (state->m_ngram_counts[i]) {
      smoothed_matches = m_match_history[i] + state->m_ngram_matches[i];
      smoothed_count = m_count_history[i] + state->m_ngram_counts[i];

      switch (m_smoothing_scheme) {
        case PLUS_ONE:
        default:
          if (i > 0) {
            // smoothing for all n > 1
        	smoothed_matches += 1;
        	smoothed_count += 1;
          }
          break;
        case LIGHT:
          if (i > 0) {
    	    // smoothing for all n > 1
        	  smoothed_matches += 0.1;
        	  smoothed_count += 0.1;
          }
          break;
        case PAPINENI:
    	  if (state->m_ngram_matches[i] == 0) {
            smooth *= 0.5;
            smoothed_matches += smooth;
            smoothed_count += smooth;
    	  }
    	  break;
      }

      precision *= smoothed_matches / smoothed_count;
    }
  }

  // take geometric mean
  precision = pow(precision, (float)1/4);

  // Apply brevity penalty if applicable.
  // BP = 1 				if c > r
  // BP = e^(1- r/c))		if c <= r
  // where
  // c: length of the candidate translation
  // r: effective reference length (sum of best match lengths for each candidate sentence)
  if (state->m_target_length < (state->m_scaled_ref_length * m_relax_BP)) {
    float smoothed_target_length = m_target_length_history + state->m_target_length;
    float smoothed_ref_length = m_ref_length_history + (state->m_scaled_ref_length * m_relax_BP);
    precision *= exp(1 - (smoothed_ref_length/ smoothed_target_length));
  }

  // Approximate bleu score as of Chiang/Resnik is scaled by the size of the input:
  // B(e;f,{r_k}) = (O_f + |f|) * BLEU(O + c(e;{r_k}))
  // where c(e;) is a vector of reference length, ngram counts and ngram matches
  if (m_scale_by_input_length) {
    precision *= m_source_length_history + m_cur_source_length;
  }
  else if (m_scale_by_ref_length) {
    precision *= m_ref_length_history + m_cur_ref_length;
  }
  else if (m_scale_by_target_length_linear) {
  	// length of current hypothesis + number of words still to translate from source (rest being translated 1-to-1)
  	float scaled_target_length = state->m_target_length + (m_cur_source_length - state->m_source_length);
  	precision *= m_target_length_history + scaled_target_length;
  }
  else if (m_scale_by_target_length_trend) {
  	// length of full target if remaining words were translated with the same fertility as so far
  	float scaled_target_length = ((float)m_cur_source_length/state->m_source_length) * state->m_target_length;
  	precision *= m_target_length_history + scaled_target_length;
  }
  else if (m_scale_by_avg_length) {
    precision *= (m_source_length_history + m_ref_length_history + m_cur_source_length +  + m_cur_ref_length) / 2;
  }
  return precision*m_scale_by_x;
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
    return new BleuScoreState();
}

} // namespace.

