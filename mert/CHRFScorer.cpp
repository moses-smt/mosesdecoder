/*
 * CHRFScorer.cpp
 *
 *  Created on: Dec 28, 2016
 *      Author: pramathur@ebay.com
 */

#include "CHRFScorer.h"
#include <fstream>
#include <stdexcept>


#include "Util.h"
#include "math.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "ScoreStats.h"
#include "util/exception.hh"
#include "Util.h"
#include "ScoreDataIterator.h"
#include "FeatureDataIterator.h"
#include "Vocabulary.h"

namespace {

const char KEY_REFLEN[] = "reflen";
const char REFLEN_AVERAGE[] = "average";
const char REFLEN_SHORTEST[] = "shortest";
const char REFLEN_CLOSEST[] = "closest";
const char KEY_BETA[] = "beta";
const char KEY_BETA_DEF[] = "3";
const char KEY_SMOOTH[] = "smooth";
const char KEY_SMOOTH_DEF[] = "0";
float BETA=3;
float SMOOTH=0;

}

namespace MosesTuning {

CHRFScorer::CHRFScorer(const std::string& config)
		  : StatisticsBasedScorer("CHRF",config), m_ref_length_type(CLOSEST), m_beta(3), m_smooth(0) {
	const std::string reflen = getConfig(KEY_REFLEN, REFLEN_CLOSEST);
	  if (reflen == REFLEN_AVERAGE) {
	    m_ref_length_type = AVERAGE;
	  } else if (reflen == REFLEN_SHORTEST) {
	    m_ref_length_type = SHORTEST;
	  } else if (reflen == REFLEN_CLOSEST) {
	    m_ref_length_type = CLOSEST;
	  } else {
	    UTIL_THROW2("Unknown reference length strategy: " + reflen);
	  }
	  const std::string beta = getConfig(KEY_BETA, KEY_BETA_DEF);
	  const std::string smooth = getConfig(KEY_SMOOTH, KEY_SMOOTH_DEF);
	  if(beta == KEY_BETA_DEF){
		  m_beta=3.0;
	  } else{
		  m_beta = ::atof(beta.c_str());
	  }
	  if(smooth == KEY_SMOOTH_DEF){
		  m_smooth=0.0;
	  }else{
		  m_smooth = ::atof(smooth.c_str());
	  }
	  BETA= m_beta;
	  SMOOTH = m_smooth;
}

CHRFScorer::~CHRFScorer() {}

void CHRFScorer::setReferenceFiles(const std::vector<std::string>& referenceFiles)
{
	// Make sure reference data is clear
	  m_references.reset();
	  mert::VocabularyFactory::GetVocabulary()->clear();

	  //load reference data
	  for (size_t i = 0; i < referenceFiles.size(); ++i) {
	    TRACE_ERR("Loading reference from " << referenceFiles[i] << std::endl);

	    std::ifstream ifs(referenceFiles[i].c_str());
	    if (!OpenReferenceStream(&ifs, i)) {
	      UTIL_THROW2("Cannot open " + referenceFiles[i]);
	    }
	  }

}

bool CHRFScorer::OpenReferenceStream(std::istream* is, size_t file_id)
{
  if (is == NULL) return false;

  std::string line;
  size_t sid = 0;
  while (getline(*is, line)) {
    // TODO: rather than loading the whole reference corpus into memory, can we stream it line by line?
    //  (loading the whole reference corpus can take gigabytes of RAM if done with millions of sentences)
    line = preprocessSentence(line);

    // chrf stuff here
    // split line into characters
    std::string temp_line;
    for(size_t i=0; i<line.size(); i++){
    if(line[i]!=' ')
        temp_line.append(line[i]+" ");
    }
    temp_line.substr(0, temp_line.size()-1);
    line = temp_line;
//    std::cerr<<line<<std::endl;

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

void CHRFScorer::ProcessReferenceLine(const std::string& line, Reference* ref) const
{
  NgramCounts counts;
  size_t length = CountNgrams(line, counts, CHRFNgramOrder);

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

size_t CHRFScorer::CountNgrams(const std::string& line, NgramCounts& counts,
                               unsigned int n, bool is_testing) const
{
  assert(n > 0);
  std::vector<int> encoded_tokens;

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
  std::vector<int> ngram;

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
//  DumpCounts(&std::cerr, counts);
  return len;
}

void CHRFScorer::prepareStats(size_t sid, const std::string& text, ScoreStats& entry)
{
  UTIL_THROW_IF2(sid >= m_references.size(), "Sentence id (" << sid << ") not found in reference set");
  CalcCHRFStats(*(m_references[sid]), text, entry);
}

void CHRFScorer::CalcCHRFStats(const Reference& ref, const std::string& text, ScoreStats& entry) const
{
  NgramCounts testcounts;
  // stats for this line
  std::vector<ScoreStatsType> stats(CHRFNgramOrder * 3);
  std::string sentence = preprocessSentence(text);
  // chrf stuff here
  // split line into characters
  std::string temp_line;
  for(size_t i=0; i<sentence.size(); i++){
	if(sentence[i]!=' ')
		temp_line.append(sentence[i]+" ");
  }
  temp_line.substr(0, temp_line.size()-1);
  sentence=temp_line;
//  std::cerr<<sentence<<std::endl;
  stats.push_back(sentence.size());
  const size_t length = CountNgrams(sentence, testcounts, CHRFNgramOrder, true);

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
      correct = std::min(v, guess);
    }
    stats[len * 3 - 3] += correct;
    stats[len * 3 - 2] += guess;
    stats[len * 3 - 1] += v;
  }
  entry.set(stats);
}

statscore_t CHRFScorer::calculateScore(const std::vector<ScoreStatsType>& comps) const
{
  UTIL_THROW_IF(comps.size() != CHRFNgramOrder * 3 + 2, util::Exception, "Error");
  float f1=0.0;
  float precision = 0.0;
  float recall = 0.0;
  for (size_t i = 0; i < CHRFNgramOrder; i++){
	  precision += ((comps[3*i] + m_smooth)*1.0) / ((comps[3*i+1] + m_smooth)*1.0);
	  recall += ((comps[3*i] + m_smooth)*1.0) / ((comps[3*i+2] + m_smooth)*1.0);
  }

  precision /= CHRFNgramOrder;
  recall /= CHRFNgramOrder;

  f1 = ((1 + pow(m_beta, 2) ) * (precision * recall) ) / ( ( pow(m_beta, 2) * precision) + recall) ;
  return f1;
}

int CHRFScorer::CalcReferenceLength(const Reference& ref, std::size_t length) const
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

void CHRFScorer::DumpCounts(std::ostream* os,
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
  *os << std::endl;
}

} /* namespace MosesTuning */
