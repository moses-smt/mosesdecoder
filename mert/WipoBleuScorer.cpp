#include "WipoBleuScorer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cctype>

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

WipoBleuScorer::WipoBleuScorer(const string& config)
  : BleuScorer("WIPOBLEU", config)
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

WipoBleuScorer::~WipoBleuScorer() {}

bool WipoBleuScorer::OpenReferenceStream(istream* is, size_t file_id)
{
  if (is == NULL) return false;

  string line;
  size_t sid = 0;
  while (getline(*is, line)) {
    line = preprocessSentence(line);
    line = preprocessWipo(line);
    
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

void WipoBleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
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
  sentence = preprocessWipo(sentence);
  
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

std::string preprocessWipo(const std::string& input) {
  std::string output = trimStr(input);
     
  // Remove sentence type markers only at beginning
  if(output[0] == '#') {
    size_t pos = output.find_first_of(" ");
    output.replace(0, pos + 1, "");
  }
  
  // Join compounds
  std::string sep = "− ";
  std::size_t found = output.find(sep);
  while (found != std::string::npos)
  {
    output.replace(found, sep.size(), "");
    found = output.find(sep, found - sep.size() + 1);
  }
  
  // Remove sentence type markers only at end
  if(output[output.size()-2] == '#') {
    output.replace(output.size()-2, 2, "");
  }
  
  // Convert to lowercase
  for (size_t i = 0; i < output.size(); ++i) {
    output[i] = std::tolower(output[i]);
  }
  
  return output;
}

}
