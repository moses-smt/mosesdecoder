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
    UTIL_THROW2("Unknown reference length strategy: " + reflen);
  }
}

BleuScorer::~BleuScorer() {}

size_t BleuScorer::CountNgrams(const string& line, NgramCounts& counts,
                               unsigned int n, bool is_testing) const
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

    ifstream ifs(referenceFiles[i].c_str());
    if (!OpenReferenceStream(&ifs, i)) {
      UTIL_THROW2("Cannot open " + referenceFiles[i]);
    }
  }
}

bool BleuScorer::OpenReferenceStream(istream* is, size_t file_id)
{
  if (is == NULL) return false;

  string line;
  size_t sid = 0;
  while (getline(*is, line)) {
    // TODO: rather than loading the whole reference corpus into memory, can we stream it line by line?
    //  (loading the whole reference corpus can take gigabytes of RAM if done with millions of sentences)
    line = preprocessSentence(line);
    if (file_id == 0) {
      Reference* ref = new Reference;
      m_references.push_back(ref);    // Take ownership of the Reference object.
    }
    UTIL_THROW_IF2(m_references.size() <= sid, "Reference " << file_id << "has too many sentences.");

    ProcessReferenceLine(line, m_references[sid]);

    if (sid > 0 && sid % 100 == 0) {
      TRACE_ERR(".");
    }
    ++sid;
  }
  return true;
}

void BleuScorer::ProcessReferenceLine(const std::string& line, Reference* ref) const
{
  NgramCounts counts;
  size_t length = CountNgrams(line, counts, kBleuNgramOrder);

  //for any counts larger than those already there, merge them in
  for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
    const NgramCounts::Key& ngram = ci->first;
    const NgramCounts::Value newcount = ci->second;

    NgramCounts::Value oldcount = 0;
    ref->get_counts()->Lookup(ngram, &oldcount);
    if (newcount > oldcount) {
      ref->get_counts()->operator[](ngram) = newcount;
    }
  }
  //add in the length
  ref->push_back(length);
}

bool BleuScorer::GetNextReferenceFromStreams(std::vector<boost::shared_ptr<std::ifstream> >& referenceStreams, Reference& ref) const
{
  for (vector<boost::shared_ptr<ifstream> >::iterator ifs=referenceStreams.begin(); ifs!=referenceStreams.end(); ++ifs) {
    if (!(*ifs)) return false;
    string line;
    if (!getline(**ifs, line)) return false;
    line = preprocessSentence(line);
    ProcessReferenceLine(line, &ref);
  }
  return true;
}

void BleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  UTIL_THROW_IF2(sid >= m_references.size(), "Sentence id (" << sid << ") not found in reference set");
  CalcBleuStats(*(m_references[sid]), text, entry);
}

void BleuScorer::CalcBleuStats(const Reference& ref, const std::string& text, ScoreStats& entry) const
{
  NgramCounts testcounts;
  // stats for this line
  vector<ScoreStatsType> stats(kBleuNgramOrder * 2);
  string sentence = preprocessSentence(text);
  const size_t length = CountNgrams(sentence, testcounts, kBleuNgramOrder, true);

  const int reference_len = CalcReferenceLength(ref, length);
  stats.push_back(reference_len);

  //precision on each ngram type
  for (NgramCounts::const_iterator testcounts_it = testcounts.begin();
       testcounts_it != testcounts.end(); ++testcounts_it) {
    const NgramCounts::Value guess = testcounts_it->second;
    const size_t len = testcounts_it->first.size();
    NgramCounts::Value correct = 0;

    NgramCounts::Value v = 0;
    if (ref.get_counts()->Lookup(testcounts_it->first, &v)) {
      correct = min(v, guess);
    }
    stats[len * 2 - 2] += correct;
    stats[len * 2 - 1] += guess;
  }
  entry.set(stats);
}

statscore_t BleuScorer::calculateScore(const vector<ScoreStatsType>& comps) const
{
  UTIL_THROW_IF(comps.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");

  float logbleu = 0.0;
  for (std::size_t i = 0; i < kBleuNgramOrder; ++i) {
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

int BleuScorer::CalcReferenceLength(const Reference& ref, std::size_t length) const
{
  switch (m_ref_length_type) {
  case AVERAGE:
    return ref.CalcAverage();
    break;
  case CLOSEST:
    return ref.CalcClosest(length);
    break;
  case SHORTEST:
    return ref.CalcShortest();
    break;
  default:
    UTIL_THROW2("Unknown reference types");
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
  for (std::size_t j = 0; j < kBleuNgramOrder; j++) {
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
  UTIL_THROW_IF(sent.size()!=bg.size(), util::Exception, "Error");
  UTIL_THROW_IF(sent.size() != kBleuNgramOrder * 2 + 1, util::Exception, "Error");
  std::vector<float> stats(sent.size());

  for(size_t i=0; i<sent.size(); i++)
    stats[i] = sent[i]+bg[i];

  // Calculate BLEU
  float logbleu = 0.0;
  for (std::size_t j = 0; j < kBleuNgramOrder; j++) {
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
  UTIL_THROW_IF2(featureDataIters[0] == FeatureDataIterator::end(),
                 "At the end of feature data iterator");
  for (size_t i = 0; i < featureFiles.size(); ++i) {
    UTIL_THROW_IF2(featureDataIters[i] == FeatureDataIterator::end(),
                   "Feature file " << i << " ended prematurely");
    UTIL_THROW_IF2(scoreDataIters[i] == ScoreDataIterator::end(),
                   "Score file " << i << " ended prematurely");
    UTIL_THROW_IF2(featureDataIters[i]->size() != scoreDataIters[i]->size(),
                   "Features and scores have different size");
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
