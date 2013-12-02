#include "BleuScoreFeature.h"

#include "moses/StaticData.h"
#include "moses/UserMessage.h"
#include "moses/Hypothesis.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{

size_t BleuScoreState::bleu_order = 4;
std::vector<BleuScoreFeature*> BleuScoreFeature::s_staticColl;

BleuScoreState::BleuScoreState(): m_words(1),
  m_source_length(0),
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

  if (StaticData::Instance().IsChart())
    return 0;

  const BleuScoreState& other = dynamic_cast<const BleuScoreState&>(o);
  int c = m_words.Compare(other.m_words);
  if (c != 0)
    return c;

  /*for(size_t i = 0; i < m_ngram_counts.size(); i++) {
    if (m_ngram_counts[i] < other.m_ngram_counts[i])
  return -1;
    if (m_ngram_counts[i] > other.m_ngram_counts[i])
  return 1;
    if (m_ngram_matches[i] < other.m_ngram_matches[i])
  return -1;
    if (m_ngram_matches[i] > other.m_ngram_matches[i])
  return 1;
  }*/

  return 0;
}
std::ostream& operator<<(std::ostream& out, const BleuScoreState& state)
{
  state.print(out);
  return out;
}

void BleuScoreState::print(std::ostream& out) const
{
  out << "ref=" << m_scaled_ref_length
      << ";source=" << m_source_length
      << ";target=" << m_target_length << ";counts=";
  for (size_t i = 0; i < bleu_order; ++i) {
    out << m_ngram_matches[i] << "/" << m_ngram_counts[i] << ",";
  }
  out << "ctxt=" << m_words;

}

void BleuScoreState::AddNgramCountAndMatches(std::vector< size_t >& counts,
    std::vector< size_t >& matches)
{
  for (size_t order = 0; order < BleuScoreState::bleu_order; ++order) {
    m_ngram_counts[order] += counts[order];
    m_ngram_matches[order] += matches[order];
  }
}


BleuScoreFeature::BleuScoreFeature(const std::string &line)
  :StatefulFeatureFunction(1, line),
   m_enabled(true),
   m_sentence_bleu(true),
   m_simple_history_bleu(false),
   m_count_history(BleuScoreState::bleu_order),
   m_match_history(BleuScoreState::bleu_order),
   m_source_length_history(0),
   m_target_length_history(0),
   m_ref_length_history(0),
   m_scale_by_input_length(true),
   m_scale_by_avg_input_length(false),
   m_scale_by_inverse_length(false),
   m_scale_by_avg_inverse_length(false),
   m_scale_by_x(1),
   m_historySmoothing(0.9),
   m_smoothing_scheme(PLUS_POINT_ONE)
{
  std::cerr << "Initializing BleuScoreFeature." << std::endl;
  s_staticColl.push_back(this);

  m_tuneable = false;

  ReadParameters();
  std::cerr << "Finished initializing BleuScoreFeature." << std::endl;
}

void BleuScoreFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "references") {
    vector<string> referenceFiles = Tokenize(value, ",");
    UTIL_THROW_IF2(referenceFiles.size() == 0, "No reference file");
    vector<vector<string> > references(referenceFiles.size());

    for (size_t i =0; i < referenceFiles.size(); ++i) {
      ifstream in(referenceFiles[i].c_str());
      if (!in) {
        stringstream strme;
        strme << "Unable to load references from " << referenceFiles[i];
        UserMessage::Add(strme.str());
        abort();
      }
      string line;
      while (getline(in,line)) {
        /*  if (GetSearchAlgorithm() == ChartDecoding) {
        stringstream tmp;
        tmp << "<s> " << line << " </s>";
        line = tmp.str();
        }
        */
        references[i].push_back(line);
      }
      if (i > 0) {
        if (references[i].size() != references[i-1].size()) {
          UserMessage::Add("Reference files are of different lengths");
          abort();
        }
      }
      in.close();
    } // for (size_t i =0; i < referenceFiles.size(); ++i) {

    //Set the references in the bleu feature
    LoadReferences(references);

  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

}

std::vector<float> BleuScoreFeature::DefaultWeights() const
{
  std::vector<float> ret(m_numScoreComponents, 1);
  return ret;
}

void BleuScoreFeature::PrintHistory(std::ostream& out) const
{
  out << "source length history=" << m_source_length_history << endl;
  out << "target length history=" << m_target_length_history << endl;
  out << "ref length history=" << m_ref_length_history << endl;

  for (size_t i = 0; i < BleuScoreState::bleu_order; ++i) {
    out << "match history/count history (" << i << "):" << m_match_history[i] << "/" << m_count_history[i] << endl;
  }
}

void BleuScoreFeature::SetBleuParameters(bool disable, bool sentenceBleu, bool scaleByInputLength, bool scaleByAvgInputLength,
    bool scaleByInverseLength, bool scaleByAvgInverseLength,
    float scaleByX, float historySmoothing, size_t scheme, bool simpleHistoryBleu)
{
  m_enabled = !disable;
  m_sentence_bleu = sentenceBleu;
  m_simple_history_bleu = simpleHistoryBleu;
  m_scale_by_input_length = scaleByInputLength;
  m_scale_by_avg_input_length = scaleByAvgInputLength;
  m_scale_by_inverse_length = scaleByInverseLength;
  m_scale_by_avg_inverse_length = scaleByAvgInverseLength;
  m_scale_by_x = scaleByX;
  m_historySmoothing = historySmoothing;
  m_smoothing_scheme = (SmoothingScheme)scheme;
}

// Incoming references (refs) are stored as refs[file_id][[sent_id][reference]]
// This data structure: m_refs[sent_id][[vector<length>][ngrams]]
void BleuScoreFeature::LoadReferences(const std::vector< std::vector< std::string > >& refs)
{
  m_refs.clear();
  FactorCollection& fc = FactorCollection::Instance();
  for (size_t file_id = 0; file_id < refs.size(); file_id++) {
    for (size_t sent_id = 0; sent_id < refs[file_id].size(); sent_id++) {
      const string& ref = refs[file_id][sent_id];
      vector<string> refTokens  = Tokenize(ref);
      if (file_id == 0)
        m_refs[sent_id] = RefValue();
      pair<vector<size_t>,NGrams>& ref_pair = m_refs[sent_id];
      (ref_pair.first).push_back(refTokens.size());
      for (size_t order = 1; order <= BleuScoreState::bleu_order; order++) {
        for (size_t end_idx = order; end_idx <= refTokens.size(); end_idx++) {
          Phrase ngram(1);
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

//	cerr << "Number of ref files: " << refs.size() << endl;
//	for (size_t i = 0; i < m_refs.size(); ++i) {
//		cerr << "Sent id " << i << ", number of references: " << (m_refs[i].first).size() << endl;
//	}
}

void BleuScoreFeature::SetCurrSourceLength(size_t source_length)
{
  m_cur_source_length = source_length;
}
void BleuScoreFeature::SetCurrNormSourceLength(size_t source_length)
{
  m_cur_norm_source_length = source_length;
}

// m_refs[sent_id][[vector<length>][ngrams]]
void BleuScoreFeature::SetCurrShortestRefLength(size_t sent_id)
{
  // look for shortest reference
  int shortestRef = -1;
  for (size_t i = 0; i < (m_refs[sent_id].first).size(); ++i) {
    if (shortestRef == -1 || (m_refs[sent_id].first)[i] < shortestRef)
      shortestRef = (m_refs[sent_id].first)[i];
  }
  m_cur_ref_length = shortestRef;
//		cerr << "Set shortest cur_ref_length: " << m_cur_ref_length << endl;
}

void BleuScoreFeature::SetCurrAvgRefLength(size_t sent_id)
{
  // compute average reference length
  size_t sum = 0;
  size_t numberRefs = (m_refs[sent_id].first).size();
  for (size_t i = 0; i < numberRefs; ++i) {
    sum += (m_refs[sent_id].first)[i];
  }
  m_cur_ref_length = (float)sum/numberRefs;
//		cerr << "Set average cur_ref_length: " << m_cur_ref_length << endl;
}

void BleuScoreFeature::SetCurrReferenceNgrams(size_t sent_id)
{
  m_cur_ref_ngrams = m_refs[sent_id].second;
}

size_t BleuScoreFeature::GetShortestRefIndex(size_t ref_id)
{
  // look for shortest reference
  int shortestRef = -1;
  size_t shortestRefIndex = 0;
  for (size_t i = 0; i < (m_refs[ref_id].first).size(); ++i) {
    if (shortestRef == -1 || (m_refs[ref_id].first)[i] < shortestRef) {
      shortestRef = (m_refs[ref_id].first)[i];
      shortestRefIndex = i;
    }
  }
  return shortestRefIndex;
}

/*
 * Update the pseudo-document O after each translation of a source sentence.
 * (O is an exponentially-weighted moving average of vectors c(e;{r_k}))
 * O = m_historySmoothing * (O + c(e_oracle))
 * O_f = m_historySmoothing * (O_f + |f|)		input length of pseudo-document
 */
void BleuScoreFeature::UpdateHistory(const vector< const Word* >& hypo)
{
  Phrase phrase(hypo);
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
void BleuScoreFeature::UpdateHistory(const vector< vector< const Word* > >& hypos, vector<size_t>& sourceLengths, vector<size_t>& ref_ids, size_t rank, size_t epoch)
{
  for (size_t ref_id = 0; ref_id < hypos.size(); ++ref_id) {
    Phrase phrase(hypos[ref_id]);
    std::vector< size_t > ngram_counts(BleuScoreState::bleu_order);
    std::vector< size_t > ngram_matches(BleuScoreState::bleu_order);

    // set current source and reference information for each oracle in the batch
    size_t cur_source_length = sourceLengths[ref_id];
    size_t hypo_length = hypos[ref_id].size();
    size_t cur_ref_length = GetClosestRefLength(ref_ids[ref_id], hypo_length);
    NGrams cur_ref_ngrams = m_refs[ref_ids[ref_id]].second;
    cerr << "reference length: " << cur_ref_length << endl;

    // compute vector c(e;{r_k}):
    // vector of effective reference length, number of ngrams in e, number of ngram matches between e and r_k
    GetNgramMatchCounts(phrase, cur_ref_ngrams, ngram_counts, ngram_matches, 0);

    // update counts and matches for every ngram length with counts from hypo
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
      m_count_history[i] += ngram_counts[i];
      m_match_history[i] += ngram_matches[i];

      // do this for last position in batch
      if (ref_id == hypos.size() - 1) {
        m_count_history[i] *= m_historySmoothing;
        m_match_history[i] *= m_historySmoothing;
      }
    }

    // update counts for reference and target length
    m_source_length_history += cur_source_length;
    m_target_length_history += hypos[ref_id].size();
    m_ref_length_history += cur_ref_length;

    // do this for last position in batch
    if (ref_id == hypos.size() - 1) {
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
/*void BleuScoreFeature::PrintReferenceLength(const vector<size_t>& ref_ids) {
	for (size_t ref_id = 0; ref_id < ref_ids.size(); ++ref_id){
	    size_t cur_ref_length = (m_refs[ref_ids[ref_id]].first)[0]; // TODO!!
	    cerr << "reference length: " << cur_ref_length << endl;
	}
}*/

size_t BleuScoreFeature::GetClosestRefLength(size_t ref_id, int hypoLength)
{
  // look for closest reference
  int currentDist = -1;
  int closestRefLength = -1;
  for (size_t i = 0; i < (m_refs[ref_id].first).size(); ++i) {
    if (closestRefLength == -1 || abs(hypoLength - (int)(m_refs[ref_id].first)[i]) < currentDist) {
      closestRefLength = (m_refs[ref_id].first)[i];
      currentDist = abs(hypoLength - (int)(m_refs[ref_id].first)[i]);
    }
  }
  return (size_t)closestRefLength;
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

// score ngrams of words that have been added before the previous word span
void BleuScoreFeature::GetNgramMatchCounts_prefix(Phrase& phrase,
    const NGrams& ref_ngram_counts,
    std::vector< size_t >& ret_counts,
    std::vector< size_t >& ret_matches,
    size_t new_start_indices,
    size_t last_end_index) const
{
  NGrams::const_iterator ref_ngram_counts_iter;
  size_t ngram_start_idx, ngram_end_idx;

  // Chiang et al (2008) use unclipped counts of ngram matches
  for (size_t start_idx = 0; start_idx < new_start_indices; start_idx++) {
    for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
      ngram_start_idx = start_idx;
      ngram_end_idx = start_idx + order;
      if (order > ngram_end_idx) break;
      if (ngram_end_idx > last_end_index) break;

      Phrase ngram = phrase.GetSubString(WordsRange(ngram_start_idx, ngram_end_idx), 0);
      ret_counts[order]++;

      ref_ngram_counts_iter = ref_ngram_counts.find(ngram);
      if (ref_ngram_counts_iter != ref_ngram_counts.end())
        ret_matches[order]++;
    }
  }
}

// score ngrams around the overlap of two previously scored phrases
void BleuScoreFeature::GetNgramMatchCounts_overlap(Phrase& phrase,
    const NGrams& ref_ngram_counts,
    std::vector< size_t >& ret_counts,
    std::vector< size_t >& ret_matches,
    size_t overlap_index) const
{
  NGrams::const_iterator ref_ngram_counts_iter;
  size_t ngram_start_idx, ngram_end_idx;

  // Chiang et al (2008) use unclipped counts of ngram matches
  for (size_t end_idx = overlap_index; end_idx < phrase.GetSize(); end_idx++) {
    if (end_idx >= (overlap_index+BleuScoreState::bleu_order-1)) break;
    for (size_t order = 0; order < BleuScoreState::bleu_order; order++) {
      if (order > end_idx) break;

      ngram_end_idx = end_idx;
      ngram_start_idx = end_idx - order;
      if (ngram_start_idx >= overlap_index) continue; // only score ngrams that span the overlap point

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
      } else {
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
  if (!m_enabled) return new BleuScoreState();

  NGrams::const_iterator reference_ngrams_iter;
  const BleuScoreState& ps = dynamic_cast<const BleuScoreState&>(*prev_state);
  BleuScoreState* new_state = new BleuScoreState(ps);

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
                      new_state->m_words.GetSize()); // number of words in previous states

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
                                   ((float)coverageVector.GetNumWordsCovered()/coverageVector.GetSize());

  // Calculate new bleu.
  new_bleu = CalculateBleu(new_state);

  // Set score to new Bleu score
  accumulator->PlusEquals(this, new_bleu - old_bleu);
  return new_state;
}

FFState* BleuScoreFeature::EvaluateChart(const ChartHypothesis& cur_hypo, int featureID,
    ScoreComponentCollection* accumulator ) const
{
  if (!m_enabled) return new BleuScoreState();

  NGrams::const_iterator reference_ngrams_iter;

  const Phrase& curr_target_phrase = static_cast<const Phrase&>(cur_hypo.GetCurrTargetPhrase());
//  cerr << "\nCur target phrase: " << cur_hypo.GetTargetLHS() << " --> " << curr_target_phrase << endl;

  // Calculate old bleu of previous states
  float old_bleu = 0, new_bleu = 0;
  size_t num_old_words = 0, num_words_first_prev = 0;
  size_t num_words_added_left = 0, num_words_added_right = 0;

  // double-check cases where more than two previous hypotheses were combined
  assert(cur_hypo.GetPrevHypos().size() <= 2);
  BleuScoreState* new_state;
  if (cur_hypo.GetPrevHypos().size() == 0)
    new_state = new BleuScoreState();
  else {
    const FFState* prev_state_zero = cur_hypo.GetPrevHypo(0)->GetFFState(featureID);
    const BleuScoreState& ps_zero = dynamic_cast<const BleuScoreState&>(*prev_state_zero);
    new_state = new BleuScoreState(ps_zero);
    num_words_first_prev = ps_zero.m_target_length;

    for (size_t i = 0; i < cur_hypo.GetPrevHypos().size(); ++i) {
      const FFState* prev_state = cur_hypo.GetPrevHypo(i)->GetFFState(featureID);
      const BleuScoreState* ps = dynamic_cast<const BleuScoreState*>(prev_state);
      BleuScoreState* ps_nonConst = const_cast<BleuScoreState*>(ps);
//  		cerr << "prev phrase: " << cur_hypo.GetPrevHypo(i)->GetOutputPhrase()
//  				<< " ( " << cur_hypo.GetPrevHypo(i)->GetTargetLHS() << ")" << endl;

      old_bleu += CalculateBleu(ps_nonConst);
      num_old_words += ps->m_target_length;

      if (i > 0)
        // add ngram matches from other previous states
        new_state->AddNgramCountAndMatches(ps_nonConst->m_ngram_counts, ps_nonConst->m_ngram_matches);
    }
  }

  // check if we are already done (don't add <s> and </s>)
  size_t numWordsCovered = cur_hypo.GetCurrSourceRange().GetNumWordsCovered();
  if (numWordsCovered == m_cur_source_length) {
    // Bleu score stays the same, do not need to add anything
    //accumulator->PlusEquals(this, 0);
    return new_state;
  }

  // set new context
  Phrase new_words = cur_hypo.GetOutputPhrase();
  new_state->m_words = new_words;
  size_t num_curr_words = new_words.GetSize();

  // get ngram matches for new words
  if (num_old_words == 0) {
//  	cerr << "compute right ngram context" << endl;
    GetNgramMatchCounts(new_words,
                        m_cur_ref_ngrams,
                        new_state->m_ngram_counts,
                        new_state->m_ngram_matches,
                        0);
  } else if (new_words.GetSize() == num_old_words) {
    // two hypotheses were glued together, compute new ngrams on the basis of first hypothesis
    num_words_added_right = num_curr_words - num_words_first_prev;
    // score around overlap point
//  	cerr << "compute overlap ngram context (" << (num_words_first_prev) << ")" << endl;
    GetNgramMatchCounts_overlap(new_words,
                                m_cur_ref_ngrams,
                                new_state->m_ngram_counts,
                                new_state->m_ngram_matches,
                                num_words_first_prev);
  } else if (num_old_words + curr_target_phrase.GetNumTerminals() == num_curr_words) {
    assert(curr_target_phrase.GetSize() == curr_target_phrase.GetNumTerminals()+1);
    // previous hypothesis + rule with 1 non-terminal were combined (NT substituted by Ts)
    for (size_t i = 0; i < curr_target_phrase.GetSize(); ++i)
      if (curr_target_phrase.GetWord(i).IsNonTerminal()) {
        num_words_added_left = i;
        num_words_added_right = curr_target_phrase.GetSize() - (i+1);
        break;
      }

    // left context
//  	cerr << "compute left ngram context" << endl;
    if (num_words_added_left > 0)
      GetNgramMatchCounts_prefix(new_words,
                                 m_cur_ref_ngrams,
                                 new_state->m_ngram_counts,
                                 new_state->m_ngram_matches,
                                 num_words_added_left,
                                 num_curr_words - num_words_added_right - 1);

    // right context
//  	cerr << "compute right ngram context" << endl;
    if (num_words_added_right > 0)
      GetNgramMatchCounts(new_words,
                          m_cur_ref_ngrams,
                          new_state->m_ngram_counts,
                          new_state->m_ngram_matches,
                          num_words_added_left + num_old_words);
  } else {
    cerr << "undefined state.. " << endl;
    exit(1);
  }

  // Update state variables
  size_t ctx_start_idx = 0;
  size_t ctx_end_idx = new_words.GetSize()-1;
  size_t bleu_context_length = BleuScoreState::bleu_order -1;
  if (ctx_end_idx > bleu_context_length) {
    ctx_start_idx = ctx_end_idx - bleu_context_length;
  }

  new_state->m_source_length = cur_hypo.GetCurrSourceRange().GetNumWordsCovered();
  new_state->m_words = new_words.GetSubString(WordsRange(ctx_start_idx, ctx_end_idx));
  new_state->m_target_length = cur_hypo.GetOutputPhrase().GetSize();

  // we need a scaled reference length to compare the current target phrase to the corresponding
  // reference phrase
  size_t cur_source_length = m_cur_source_length;
  new_state->m_scaled_ref_length = m_cur_ref_length * (float(new_state->m_source_length)/cur_source_length);

  // Calculate new bleu.
  new_bleu = CalculateBleu(new_state);

  // Set score to new Bleu score
  accumulator->PlusEquals(this, new_bleu - old_bleu);
  return new_state;
}

/**
 * Calculate real sentence Bleu score of complete translation
 */
float BleuScoreFeature::CalculateBleu(Phrase translation) const
{
  if (translation.GetSize() == 0)
    return 0.0;

  Phrase normTranslation = translation;
  // remove start and end symbol for chart decoding
  if (m_cur_source_length != m_cur_norm_source_length) {
    WordsRange* range = new WordsRange(1, translation.GetSize()-2);
    normTranslation = translation.GetSubString(*range);
  }

  // get ngram matches for translation
  BleuScoreState* state = new BleuScoreState();
  GetClippedNgramMatchesAndCounts(normTranslation,
                                  m_cur_ref_ngrams,
                                  state->m_ngram_counts,
                                  state->m_ngram_matches,
                                  0); // number of words in previous states

  // set state variables
  state->m_words = normTranslation;
  state->m_source_length = m_cur_norm_source_length;
  state->m_target_length = normTranslation.GetSize();
  state->m_scaled_ref_length = m_cur_ref_length;

  // Calculate bleu.
  return CalculateBleu(state);
}

/*
 * Calculate Bleu score for a partial hypothesis given as state.
 */
float BleuScoreFeature::CalculateBleu(BleuScoreState* state) const
{
  if (!state->m_ngram_counts[0]) return 0;
  if (!state->m_ngram_matches[0]) return 0;      	// if we have no unigram matches, score should be 0

  float precision = 1.0;
  float smooth = 1;
  float smoothed_count, smoothed_matches;

  if (m_sentence_bleu || m_simple_history_bleu) {
    // Calculate geometric mean of modified ngram precisions
    // BLEU = BP * exp(SUM_1_4 1/4 * log p_n)
    // 		= BP * 4th root(PRODUCT_1_4 p_n)
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
      if (state->m_ngram_counts[i]) {
        smoothed_matches = state->m_ngram_matches[i];
        smoothed_count = state->m_ngram_counts[i];

        switch (m_smoothing_scheme) {
        case PLUS_ONE:
        default:
          if (i > 0) {
            // smoothing for all n > 1
            smoothed_matches += 1;
            smoothed_count += 1;
          }
          break;
        case PLUS_POINT_ONE:
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

        if (m_simple_history_bleu) {
          smoothed_matches += m_match_history[i];
          smoothed_count += m_count_history[i];
        }

        precision *= smoothed_matches/smoothed_count;
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
    if (m_simple_history_bleu) {
      if ((m_target_length_history + state->m_target_length) < (m_ref_length_history + state->m_scaled_ref_length)) {
        float smoothed_target_length = m_target_length_history + state->m_target_length;
        float smoothed_ref_length = m_ref_length_history + state->m_scaled_ref_length;
        precision *= exp(1 - (smoothed_ref_length/smoothed_target_length));
      }
    } else {
      if (state->m_target_length < state->m_scaled_ref_length) {
        float target_length = state->m_target_length;
        float ref_length = state->m_scaled_ref_length;
        precision *= exp(1 - (ref_length/target_length));
      }
    }

    //cerr << "precision: " << precision << endl;

    // Approximate bleu score as of Chiang/Resnik is scaled by the size of the input:
    // B(e;f,{r_k}) = (O_f + |f|) * BLEU(O + c(e;{r_k}))
    // where c(e;) is a vector of reference length, ngram counts and ngram matches
    if (m_scale_by_input_length) {
      precision *= m_cur_norm_source_length;
    } else if (m_scale_by_avg_input_length) {
      precision *= m_avg_input_length;
    } else if (m_scale_by_inverse_length) {
      precision *= (100/m_cur_norm_source_length);
    } else if (m_scale_by_avg_inverse_length) {
      precision *= (100/m_avg_input_length);
    }

    return precision * m_scale_by_x;
  } else {
    // Revised history BLEU: compute Bleu in the context of the pseudo-document
    // B(b) = size_of_oracle_doc * (Bleu(B_hist + b) - Bleu(B_hist))
    // Calculate geometric mean of modified ngram precisions
    // BLEU = BP * exp(SUM_1_4 1/4 * log p_n)
    // 		= BP * 4th root(PRODUCT_1_4 p_n)
    for (size_t i = 0; i < BleuScoreState::bleu_order; i++) {
      if (state->m_ngram_counts[i]) {
        smoothed_matches = m_match_history[i] + state->m_ngram_matches[i] + 0.1;
        smoothed_count = m_count_history[i] + state->m_ngram_counts[i] + 0.1;
        precision *= smoothed_matches/smoothed_count;
      }
    }

    // take geometric mean
    precision = pow(precision, (float)1/4);

    // Apply brevity penalty if applicable.
    if ((m_target_length_history + state->m_target_length) < (m_ref_length_history + state->m_scaled_ref_length))
      precision *= exp(1 - ((m_ref_length_history + state->m_scaled_ref_length)/(m_target_length_history + state->m_target_length)));

    cerr << "precision: " << precision << endl;

    // **BLEU score of pseudo-document**
    float precision_pd = 1.0;
    if (m_target_length_history > 0) {
      for (size_t i = 0; i < BleuScoreState::bleu_order; i++)
        if (m_count_history[i] != 0)
          precision_pd *= (m_match_history[i] + 0.1)/(m_count_history[i] + 0.1);

      // take geometric mean
      precision_pd = pow(precision_pd, (float)1/4);

      // Apply brevity penalty if applicable.
      if (m_target_length_history < m_ref_length_history)
        precision_pd *= exp(1 - (m_ref_length_history/m_target_length_history));
    } else
      precision_pd = 0;
    // **end BLEU of pseudo-document**

    cerr << "precision pd: " << precision_pd << endl;

    float sentence_impact;
    if (m_target_length_history > 0)
      sentence_impact = m_target_length_history * (precision - precision_pd);
    else
      sentence_impact = precision;

    cerr << "sentence impact: " << sentence_impact << endl;
    return sentence_impact * m_scale_by_x;
  }
}

const FFState* BleuScoreFeature::EmptyHypothesisState(const InputType& input) const
{
  return new BleuScoreState();
}

bool BleuScoreFeature::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[0];
  return 0;
}

} // namespace.

