#ifndef __SEMPOSSCORER_H__
#define __SEMPOSSCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits.h>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"

using namespace std;

const int maxNOC = 30;

typedef pair<string,string> str_item_t;
typedef vector<str_item_t> str_sentence_t;
typedef str_sentence_t::const_iterator str_sentence_it;

typedef pair<int,int> item_t;
typedef multiset<item_t> sentence_t;
typedef sentence_t::const_iterator sentence_it;

// Base class for classes representing overlapping formulas
class SemposOverlapping
{
public:
  virtual vector<int> prepareStats(const sentence_t& cand, const sentence_t& ref) = 0;
  virtual float calculateScore(const vector<int>& stats) = 0;
  virtual size_t NumberOfScores() const = 0;
};

// Overlapping proposed by (Bojar and Machacek,2011);
class CapMicroOverlapping : public SemposOverlapping
{
public:
  virtual vector<int> prepareStats(const sentence_t& cand, const sentence_t& ref);
  virtual float calculateScore(const vector<int>& stats);
  virtual size_t NumberOfScores() const {
    return 2;
  }
};

//Overlapping proposed by (Bojar and Kos,2009)
class CapMacroOverlapping : public SemposOverlapping
{
public:
  virtual vector<int> prepareStats(const sentence_t& cand, const sentence_t& ref);
  virtual float calculateScore(const vector<int>& stats);
  virtual size_t NumberOfScores() const {
    return maxNOC*2;
  }
};


// This class represents sempos based metrics
class SemposScorer: public StatisticsBasedScorer
{
public:
  SemposScorer(const string& config);
  virtual void setReferenceFiles(const vector<string>& referenceFiles);
  virtual void prepareStats(size_t sindex, const string& text, ScoreStats& entry);

  virtual size_t NumberOfScores() const {
    return ovr->NumberOfScores();
  };


  ~SemposScorer();

  virtual float calculateScore(const vector<int>& comps) const;

private:
  SemposOverlapping* ovr;
  vector<vector<sentence_t> > ref_sentences;

  typedef map<string, int> encoding_t;
  typedef encoding_t::iterator   encoding_it;

  encoding_t semposMap;
  encoding_t stringMap;

  void splitSentence(const string& sentence, str_sentence_t& splitSentence);
  void encodeSentence(const str_sentence_t& sentence, sentence_t& encodedSentence);
  int encodeString(const string& str);
  int encodeSempos(const string& sempos);

  //no copy
  SemposScorer(const SemposScorer&);
  SemposScorer& operator=(const SemposScorer&);

  bool debug;
};

#endif //__BLEUSCORER_H
