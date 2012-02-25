#include "BleuScorer.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "Util.h"

namespace {

// configure regularisation
const char KEY_REFLEN[] = "reflen";
const char REFLEN_AVERAGE[] = "average";
const char REFLEN_SHORTEST[] = "shortest";
const char REFLEN_CLOSEST[] = "closest";

} // namespace

// A simple STL-map based n-gram counts.
// Basically, we provide typical accessors and mutaors, but
// we intentionally does not allow erasing elements.
class BleuScorer::NgramCounts {
 public:
  // Used to construct the ngram map
  struct NgramComparator {
    bool operator()(const vector<int>& a, const vector<int>& b) const {
      size_t i;
      const size_t as = a.size();
      const size_t bs = b.size();
      for (i = 0; i < as && i < bs; ++i) {
        if (a[i] < b[i]) {
          return true;
        }
        if (a[i] > b[i]) {
          return false;
        }
      }
      // entries are equal, shortest wins
      return as < bs;
    }
  };

  typedef vector<int> Key;
  typedef int Value;
  typedef map<Key, Value, NgramComparator>::iterator iterator;
  typedef map<Key, Value, NgramComparator>::const_iterator const_iterator;

  NgramCounts() : kDefaultCount(1) { }
  virtual ~NgramCounts() { }

  // If the specified "ngram" is found, we add counts.
  // If not, we insert the default count in the container.
  void add(const Key& ngram) {
    const_iterator it = find(ngram);
    if (it != end()) {
      m_counts[ngram] = it->second + 1;
    } else {
      m_counts[ngram] = kDefaultCount;
    }
  }

  void clear() { m_counts.clear(); }

  bool empty() const { return m_counts.empty(); }

  size_t size() const { return m_counts.size(); }
  size_t max_size() const { return m_counts.max_size(); }

  iterator find(const Key& ngram) { return m_counts.find(ngram); }
  const_iterator find(const Key& ngram) const { return m_counts.find(ngram); }

  Value& operator[](const Key& ngram) { return m_counts[ngram]; }

  iterator begin() { return m_counts.begin(); }
  const_iterator begin() const { return m_counts.begin(); }
  iterator end() { return m_counts.end(); }
  const_iterator end() const { return m_counts.end(); }

 private:
  const int kDefaultCount;
  map<Key, Value, NgramComparator> m_counts;
};

BleuScorer::BleuScorer(const string& config)
    : StatisticsBasedScorer("BLEU", config),
      kLENGTH(4),
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
      size_t length = countNgrams(line, counts, kLENGTH);

      //for any counts larger than those already there, merge them in
      for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
        NgramCounts::const_iterator oldcount_it = m_ref_counts[sid]->find(ci->first);
        int oldcount = 0;
        if (oldcount_it != m_ref_counts[sid]->end()) {
          oldcount = oldcount_it->second;
        }
        int newcount = ci->second;
        if (newcount > oldcount) {
          m_ref_counts[sid]->operator[](ci->first) = newcount;
        }
      }
      //add in the length
      m_ref_lengths[sid].push_back(length);
      if (sid > 0 && sid % 100 == 0) {
        TRACE_ERR(".");
      }
      ++sid;
    }
    TRACE_ERR(endl);
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
  vector<ScoreStatsType> stats(kLENGTH * 2);;
  const size_t length = countNgrams(text, testcounts, kLENGTH);

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
    NgramCounts::const_iterator refcounts_it = m_ref_counts[sid]->find(testcounts_it->first);
    int correct = 0;
    const int guess = testcounts_it->second;
    if (refcounts_it != m_ref_counts[sid]->end()) {
      correct = min(refcounts_it->second,guess);
    }
    const size_t len = testcounts_it->first.size();
    stats[len*2-2] += correct;
    stats[len*2-1] += guess;
  }
  entry.set(stats);
}

float BleuScorer::calculateScore(const vector<int>& comps) const
{
  float logbleu = 0.0;
  for (int i = 0; i < kLENGTH; ++i) {
    if (comps[2*i] == 0) {
      return 0.0;
    }
    logbleu += log(comps[2*i]) - log(comps[2*i+1]);

  }
  logbleu /= kLENGTH;
  const float brevity = 1.0 - static_cast<float>(comps[kLENGTH*2]) / comps[1];//reflength divided by test length
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
