#include "BleuScorer.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "Ngram.h"
#include "Util.h"

namespace {

// configure regularisation
const char KEY_REFLEN[] = "reflen";
const char REFLEN_AVERAGE[] = "average";
const char REFLEN_SHORTEST[] = "shortest";
const char REFLEN_CLOSEST[] = "closest";

} // namespace


BleuScorer::BleuScorer(const string& config)
    : StatisticsBasedScorer("BLEU", config),
      m_ref_length_type(CLOSEST) {
  const string reflen = getConfig(KEY_REFLEN, REFLEN_CLOSEST);
  if (reflen == REFLEN_AVERAGE) {
    m_ref_length_type = AVERAGE;
  } else if (reflen == REFLEN_SHORTEST) {
    m_ref_length_type = SHORTEST;
  } else if (reflen == REFLEN_CLOSEST) {
    m_ref_length_type = CLOSEST;
  } else {
    throw runtime_error("Unknown reference length strategy: " + reflen);
  }
}

BleuScorer::~BleuScorer() {}

size_t BleuScorer::countNgrams(const string& line, NgramCounts& counts,
                               unsigned int n)
{
  vector<int> encoded_tokens;
  TokenizeAndEncode(line, encoded_tokens);
  for (size_t k = 1; k <= n; ++k) {
    //ngram order longer than sentence - no point
    if (k > encoded_tokens.size()) {
      continue;
    }
    for (size_t i = 0; i < encoded_tokens.size()-k+1; ++i) {
      vector<int> ngram;
      for (size_t j = i; j < i+k && j < encoded_tokens.size(); ++j) {
        ngram.push_back(encoded_tokens[j]);
      }
      counts.add(ngram);
    }
  }
  return encoded_tokens.size();
}

void BleuScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  //make sure reference data is clear
  m_ref_counts.reset();
  m_ref_lengths.clear();
  ClearEncoder();

  //load reference data
  for (size_t i = 0; i < referenceFiles.size(); ++i) {
    TRACE_ERR("Loading reference from " << referenceFiles[i] << endl);
    ifstream refin(referenceFiles[i].c_str());
    if (!refin) {
      throw runtime_error("Unable to open: " + referenceFiles[i]);
    }
    string line;
    size_t sid = 0; //sentence counter
    while (getline(refin,line)) {
      line = this->applyFactors(line);
      if (i == 0) {
        NgramCounts *counts = new NgramCounts; //these get leaked
        m_ref_counts.push_back(counts);
        vector<size_t> lengths;
        m_ref_lengths.push_back(lengths);
      }
      if (m_ref_counts.size() <= sid) {
        throw runtime_error("File " + referenceFiles[i] + " has too many sentences");
      }
      NgramCounts counts;
      size_t length = countNgrams(line, counts, kBleuNgramOrder);

      //for any counts larger than those already there, merge them in
      for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
        const NgramCounts::Key& ngram = ci->first;
        const NgramCounts::Value newcount = ci->second;

        NgramCounts::Value oldcount = 0;
        m_ref_counts[sid]->lookup(ngram, &oldcount);
        if (newcount > oldcount) {
          m_ref_counts[sid]->operator[](ngram) = newcount;
        }
      }
      //add in the length
      m_ref_lengths[sid].push_back(length);
      if (sid > 0 && sid % 100 == 0) {
        TRACE_ERR(".");
      }
      ++sid;
    }
  }
}


void BleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  if (sid >= m_ref_counts.size()) {
    stringstream msg;
    msg << "Sentence id (" << sid << ") not found in reference set";
    throw runtime_error(msg.str());
  }
  NgramCounts testcounts;
  // stats for this line
  vector<ScoreStatsType> stats(kBleuNgramOrder * 2);
  string sentence = this->applyFactors(text);
  const size_t length = countNgrams(sentence, testcounts, kBleuNgramOrder);

  // Calculate effective reference length.
  switch (m_ref_length_type) {
    case SHORTEST:
      CalcShortest(sid, stats);
      break;
    case AVERAGE:
      CalcAverage(sid, stats);
      break;
    case CLOSEST:
      CalcClosest(sid, length, stats);
      break;
    default:
      throw runtime_error("Unsupported reflength strategy");
  }

  //precision on each ngram type
  for (NgramCounts::const_iterator testcounts_it = testcounts.begin();
       testcounts_it != testcounts.end(); ++testcounts_it) {
    const NgramCounts::Value guess = testcounts_it->second;
    const size_t len = testcounts_it->first.size();
    NgramCounts::Value correct = 0;

    NgramCounts::Value v = 0;
    if (m_ref_counts[sid]->lookup(testcounts_it->first, &v)) {
      correct = min(v, guess);
    }
    stats[len * 2 - 2] += correct;
    stats[len * 2 - 1] += guess;
  }
  entry.set(stats);
}

float BleuScorer::calculateScore(const vector<int>& comps) const
{
  float logbleu = 0.0;
  for (int i = 0; i < kBleuNgramOrder; ++i) {
    if (comps[2*i] == 0) {
      return 0.0;
    }
    logbleu += log(comps[2*i]) - log(comps[2*i+1]);

  }
  logbleu /= kBleuNgramOrder;
  // reflength divided by test length
  const float brevity = 1.0 - static_cast<float>(comps[kBleuNgramOrder * 2]) / comps[1];
  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}

void BleuScorer::dump_counts(ostream* os,
                             const NgramCounts& counts) const {
  for (NgramCounts::const_iterator it = counts.begin();
       it != counts.end(); ++it) {
    *os << "(";
    const NgramCounts::Key& keys = it->first;
    for (size_t i = 0; i < keys.size(); ++i) {
      if (i != 0) {
        *os << " ";
      }
      *os << keys[i];
    }
    *os << ") : " << it->second << ", ";
  }
  *os << endl;
}

void BleuScorer::CalcAverage(size_t sentence_id,
                             vector<ScoreStatsType>& stats) const {
  int total = 0;
  for (size_t i = 0;
       i < m_ref_lengths[sentence_id].size(); ++i) {
    total += m_ref_lengths[sentence_id][i];
  }
  const float mean = static_cast<float>(total) /
                     m_ref_lengths[sentence_id].size();
  stats.push_back(static_cast<ScoreStatsType>(mean));
}

void BleuScorer::CalcClosest(size_t sentence_id,
                             size_t length,
                             vector<ScoreStatsType>& stats) const {
  int min_diff = INT_MAX;
  int min_idx = 0;
  for (size_t i = 0; i < m_ref_lengths[sentence_id].size(); ++i) {
    const int reflength = m_ref_lengths[sentence_id][i];
    const int length_diff = abs(reflength - static_cast<int>(length));

    // Look for the closest reference
    if (length_diff < abs(min_diff)) {
      min_diff = reflength - length;
      min_idx = i;
    // if two references has the same closest length, take the shortest
    } else if (length_diff == abs(min_diff)) {
      if (reflength < static_cast<int>(m_ref_lengths[sentence_id][min_idx])) {
        min_idx = i;
      }
    }
  }
  stats.push_back(m_ref_lengths[sentence_id][min_idx]);
}

void BleuScorer::CalcShortest(size_t sentence_id,
                              vector<ScoreStatsType>& stats) const {
  const int shortest = *min_element(m_ref_lengths[sentence_id].begin(),
                                    m_ref_lengths[sentence_id].end());
  stats.push_back(shortest);
}
