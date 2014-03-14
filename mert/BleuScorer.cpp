#include "BleuScorer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "util/exception.hh"
#include "Ngram.h"
#include "Reference.h"
#include "Util.h"
#include "ScoreDataIterator.h"
#include "FeatureDataIterator.h"
#include "Vocabulary.h"

using namespace std;

namespace
{

// configure regularisation
const char KEY_REFLEN[] = "reflen";
const char REFLEN_AVERAGE[] = "average";
const char REFLEN_SHORTEST[] = "shortest";
const char REFLEN_CLOSEST[] = "closest";

} // namespace

namespace MosesTuning
{


BleuScorer::BleuScorer(const string& config)
  : StatisticsBasedScorer("BLEU", config),
    m_ref_length_type(CLOSEST)
{
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

size_t BleuScorer::CountNgrams(const string& line, NgramCounts& counts,
                               unsigned int n, bool is_testing)
{
  assert(n > 0);
  vector<int> encoded_tokens;

  // When performing tokenization of a hypothesis translation, we don't have
  // to update the Scorer's word vocabulary. However, the tokenization of
  // reference translations requires modifying the vocabulary, which means
  // this procedure might be slower than the tokenization the hypothesis
  // translation.
  if (is_testing) {
    TokenizeAndEncodeTesting(line, encoded_tokens);
  } else {
    TokenizeAndEncode(line, encoded_tokens);
  }
  const size_t len = encoded_tokens.size();
  vector<int> ngram;

  for (size_t k = 1; k <= n; ++k) {
    //ngram order longer than sentence - no point
    if (k > len) {
      continue;
    }
    for (size_t i = 0; i < len - k + 1; ++i) {
      ngram.clear();
      ngram.reserve(len);
      for (size_t j = i; j < i+k && j < len; ++j) {
        ngram.push_back(encoded_tokens[j]);
      }
      counts.Add(ngram);
    }
  }
  return len;
}

void BleuScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  // Make sure reference data is clear
  m_references.reset();
  mert::VocabularyFactory::GetVocabulary()->clear();

  //load reference data
  for (size_t i = 0; i < referenceFiles.size(); ++i) {
    TRACE_ERR("Loading reference from " << referenceFiles[i] << endl);

    if (!OpenReference(referenceFiles[i].c_str(), i)) {
      throw runtime_error("Unable to open " + referenceFiles[i]);
    }
  }
}

bool BleuScorer::OpenReference(const char* filename, size_t file_id)
{
  ifstream ifs(filename);
  if (!ifs) {
    cerr << "Cannot open " << filename << endl;
    return false;
  }
  return OpenReferenceStream(&ifs, file_id);
}

bool BleuScorer::OpenReferenceStream(istream* is, size_t file_id)
{
  if (is == NULL) return false;

  string line;
  size_t sid = 0;
  while (getline(*is, line)) {
    line = preprocessSentence(line);
    if (file_id == 0) {
      Reference* ref = new Reference;
      m_references.push_back(ref);    // Take ownership of the Reference object.
    }
    if (m_references.size() <= sid) {
      cerr << "Reference " << file_id << "has too many sentences." << endl;
      return false;
    }
    NgramCounts counts;
    size_t length = CountNgrams(line, counts, kBleuNgramOrder);

    //for any counts larger than those already there, merge them in
    for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
      const NgramCounts::Key& ngram = ci->first;
      const NgramCounts::Value newcount = ci->second;

      NgramCounts::Value oldcount = 0;
      m_references[sid]->get_counts()->Lookup(ngram, &oldcount);
      if (newcount > oldcount) {
        m_references[sid]->get_counts()->operator[](ngram) = newcount;
      }
    }
    //add in the length
    m_references[sid]->push_back(length);
    if (sid > 0 && sid % 100 == 0) {
      TRACE_ERR(".");
    }
    ++sid;
  }
  return true;
}

void BleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  if (sid >= m_references.size()) {
    stringstream msg;
    msg << "Sentence id (" << sid << ") not found in reference set";
    throw runtime_error(msg.str());
  }
  NgramCounts testcounts;
  // stats for this line
  vector<ScoreStatsType> stats(kBleuNgramOrder * 2);
  string sentence = preprocessSentence(text);
  const size_t length = CountNgrams(sentence, testcounts, kBleuNgramOrder, true);

  const int reference_len = CalcReferenceLength(sid, length);
  stats.push_back(reference_len);

  //precision on each ngram type
  for (NgramCounts::const_iterator testcounts_it = testcounts.begin();
       testcounts_it != testcounts.end(); ++testcounts_it) {
    const NgramCounts::Value guess = testcounts_it->second;
    const size_t len = testcounts_it->first.size();
    NgramCounts::Value correct = 0;

    NgramCounts::Value v = 0;
    if (m_references[sid]->get_counts()->Lookup(testcounts_it->first, &v)) {
      correct = min(v, guess);
    }
    stats[len * 2 - 2] += correct;
    stats[len * 2 - 1] += guess;
  }
  entry.set(stats);
}

statscore_t BleuScorer::calculateScore(const vector<int>& comps) const
{
  UTIL_THROW_IF(comps.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");

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

int BleuScorer::CalcReferenceLength(size_t sentence_id, size_t length)
{
  switch (m_ref_length_type) {
  case AVERAGE:
    return m_references[sentence_id]->CalcAverage();
    break;
  case CLOSEST:
    return m_references[sentence_id]->CalcClosest(length);
    break;
  case SHORTEST:
    return m_references[sentence_id]->CalcShortest();
    break;
  default:
    cerr << "unknown reference types." << endl;
    exit(1);
  }
}

void BleuScorer::DumpCounts(ostream* os,
                            const NgramCounts& counts) const
{
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

float smoothedSentenceBleu
(const std::vector<float>& stats, float smoothing, bool smoothBP)
{
  UTIL_THROW_IF(stats.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");

  float logbleu = 0.0;
  for (int j = 0; j < kBleuNgramOrder; j++) {
    logbleu += log(stats[2 * j] + smoothing) - log(stats[2 * j + 1] + smoothing);
  }
  logbleu /= kBleuNgramOrder;
  const float reflength = stats[(kBleuNgramOrder * 2)]  +
                          (smoothBP ? smoothing : 0.0f);
  const float brevity = 1.0 - reflength / stats[1];

  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}

float sentenceLevelBackgroundBleu(const std::vector<float>& sent, const std::vector<float>& bg)
{
  // Sum sent and background
  std::vector<float> stats;
  UTIL_THROW_IF(sent.size()!=bg.size(), util::Exception, "Error");
  UTIL_THROW_IF(sent.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");

  for(size_t i=0; i<sent.size(); i++)
    stats.push_back(sent[i]+bg[i]);

  // Calculate BLEU
  float logbleu = 0.0;
  for (int j = 0; j < kBleuNgramOrder; j++) {
    logbleu += log(stats[2 * j]) - log(stats[2 * j + 1]);
  }
  logbleu /= kBleuNgramOrder;
  const float brevity = 1.0 - stats[(kBleuNgramOrder * 2)] / stats[1];

  if (brevity < 0.0) {
    logbleu += brevity;
  }

  // Exponentiate and scale by reference length (as per Chiang et al 08)
  return exp(logbleu) * stats[kBleuNgramOrder*2];
}

float unsmoothedBleu(const std::vector<float>& stats)
{
  UTIL_THROW_IF(stats.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");

  float logbleu = 0.0;
  for (int j = 0; j < kBleuNgramOrder; j++) {
    logbleu += log(stats[2 * j]) - log(stats[2 * j + 1]);
  }
  logbleu /= kBleuNgramOrder;
  const float brevity = 1.0 - stats[(kBleuNgramOrder * 2)] / stats[1];

  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}

vector<float> BleuScorer::ScoreNbestList(const string& scoreFile, const string& featureFile)
{
  vector<string> scoreFiles;
  vector<string> featureFiles;
  scoreFiles.push_back(scoreFile);
  featureFiles.push_back(featureFile);

  vector<FeatureDataIterator> featureDataIters;
  vector<ScoreDataIterator> scoreDataIters;
  for (size_t i = 0; i < featureFiles.size(); ++i) {
    featureDataIters.push_back(FeatureDataIterator(featureFiles[i]));
    scoreDataIters.push_back(ScoreDataIterator(scoreFiles[i]));
  }

  vector<pair<size_t,size_t> > hypotheses;
  if (featureDataIters[0] == FeatureDataIterator::end()) {
    cerr << "Error: at the end of feature data iterator" << endl;
    exit(1);
  }
  for (size_t i = 0; i < featureFiles.size(); ++i) {
    if (featureDataIters[i] == FeatureDataIterator::end()) {
      cerr << "Error: Feature file " << i << " ended prematurely" << endl;
      exit(1);
    }
    if (scoreDataIters[i] == ScoreDataIterator::end()) {
      cerr << "Error: Score file " << i << " ended prematurely" << endl;
      exit(1);
    }
    if (featureDataIters[i]->size() != scoreDataIters[i]->size()) {
      cerr << "Error: features and scores have different size" << endl;
      exit(1);
    }
    for (size_t j = 0; j < featureDataIters[i]->size(); ++j) {
      hypotheses.push_back(pair<size_t,size_t>(i,j));
    }
  }

  // score the nbest list
  vector<float> bleuScores;
  for (size_t i=0; i < hypotheses.size(); ++i) {
    pair<size_t,size_t> translation = hypotheses[i];
    float bleu = smoothedSentenceBleu(scoreDataIters[translation.first]->operator[](translation.second));
    bleuScores.push_back(bleu);
  }
  return bleuScores;
}



}
