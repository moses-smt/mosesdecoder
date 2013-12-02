#include "BleuDocScorer.h"

#include <sys/types.h>
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


BleuDocScorer::BleuDocScorer(const string& config)
  : BleuScorer("BLEUDOC", config),
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

BleuDocScorer::~BleuDocScorer() {}


bool BleuDocScorer::OpenReferenceStream(istream* is, size_t file_id)
{
  if (is == NULL) return false;

  string line;
  size_t doc_id = -1;
  size_t sid = 0;
  while (getline(*is, line)) {

    if (line.find("<doc docid") != std::string::npos) {  // new document
      doc_id++;
      m_references.push_back(new ScopedVector<Reference>());
      sid = 0;
    } else if (line.find("<seg") != std::string::npos) { //new sentence
      int start = line.find_first_of('>') + 1;
      std::string trans = line.substr(start, line.find_last_of('<')-start);
      trans = preprocessSentence(trans);

      if (file_id == 0) {
        Reference* ref = new Reference;
        m_references[doc_id]->push_back(ref);    // Take ownership of the Reference object.
      }

      if (m_references[doc_id]->size() <= sid) {
        return false;
      }
      NgramCounts counts;
      size_t length = CountNgrams(trans, counts, kBleuNgramOrder);

      //for any counts larger than those already there, merge them in
      for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
        const NgramCounts::Key& ngram = ci->first;
        const NgramCounts::Value newcount = ci->second;

        NgramCounts::Value oldcount = 0;
        m_references[doc_id]->get().at(sid)->get_counts()->Lookup(ngram, &oldcount);
        if (newcount > oldcount) {
          m_references[doc_id]->get().at(sid)->get_counts()->operator[](ngram) = newcount;
        }
      }
      //add in the length

      m_references[doc_id]->get().at(sid)->push_back(length);
      if (sid > 0 && sid % 100 == 0) {
        TRACE_ERR(".");
      }
      ++sid;
    }
  }
  return true;
}

void BleuDocScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  if (sid >= m_references.size()) {
    stringstream msg;
    msg << "Sentence id (" << sid << ") not found in reference set";
    throw runtime_error(msg.str());
  }

  std::vector<std::string> sentences = splitDoc(text);

  vector<ScoreStatsType> totStats(kBleuNgramOrder * 2 + 1);

  for (uint i=0; i<sentences.size(); ++i) {

    NgramCounts testcounts;
    // stats for this line
    vector<ScoreStatsType> stats(kBleuNgramOrder * 2);
    string sentence = preprocessSentence(sentences[i]);
    const size_t length = CountNgrams(sentence, testcounts, kBleuNgramOrder);

    //precision on each ngram type
    for (NgramCounts::const_iterator testcounts_it = testcounts.begin();
         testcounts_it != testcounts.end(); ++testcounts_it) {
      const NgramCounts::Value guess = testcounts_it->second;
      const size_t len = testcounts_it->first.size();
      NgramCounts::Value correct = 0;

      NgramCounts::Value v = 0;
      if (m_references[sid]->get().at(i)->get_counts()->Lookup(testcounts_it->first, &v)) {
        correct = min(v, guess);
      }
      stats[len * 2 - 2] += correct;
      stats[len * 2 - 1] += guess;
    }

    const int reference_len = CalcReferenceLength(sid, i, length);
    stats.push_back(reference_len);

    //ADD stats to totStats
    std::transform(stats.begin(), stats.end(), totStats.begin(),
                   totStats.begin(), std::plus<int>());
  }
  entry.set(totStats);
}

std::vector<std::string> BleuDocScorer::splitDoc(const std::string& text)
{
  std::vector<std::string> res;

  uint index = 0;
  std::string::size_type end;

  while ((end = text.find(" \\n ", index)) != std::string::npos) {
    res.push_back(text.substr(index,end-index));
    index = end + 4;
  }
  return res;
}

statscore_t BleuDocScorer::calculateScore(const vector<int>& comps) const
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

int BleuDocScorer::CalcReferenceLength(size_t doc_id, size_t sentence_id, size_t length)
{
  switch (m_ref_length_type) {
  case AVERAGE:
    return m_references[doc_id]->get().at(sentence_id)->CalcAverage();
    break;
  case CLOSEST:
    return m_references[doc_id]->get().at(sentence_id)->CalcClosest(length);
    break;
  case SHORTEST:
    return m_references[doc_id]->get().at(sentence_id)->CalcShortest();
    break;
  default:
    cerr << "unknown reference types." << endl;
    exit(1);
  }
}

}

