#include "M2Scorer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include <boost/lexical_cast.hpp>


using namespace std;
using namespace boost::python;

namespace MosesTuning
{

M2Scorer::M2Scorer(const string& config)
  : StatisticsBasedScorer("M2Scorer", config),
    beta_(Scan<float>(getConfig("beta", "0.5"))),
    max_unchanged_words_(Scan<int>(getConfig("max_unchanged_words", "2"))),
    truecase_(Scan<bool>(getConfig("case", "false"))),
    m2_(max_unchanged_words_, beta_, truecase_)
{}

void M2Scorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  for(size_t i = 0; i < referenceFiles.size(); ++i) {
    m2_.ReadM2(referenceFiles[i]);
    break;
  }
}

void M2Scorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  string sentence = trimStr(this->preprocessSentence(text));
  
  /*
  std::pair<size_t, string> sidSent(sid, sentence);
  if(seen_.count(sidSent) != 0) {
    entry.set(seen_[sidSent]);
    return;
  }*/
  
  std::vector<ScoreStatsType> stats(4, 0);
  m2_.SufStats(sentence, sid, stats);
  
  //seen_[sidSent] = stats;
  entry.set(stats);
}

float M2Scorer::calculateScore(const vector<ScoreStatsType>& comps) const
{
  if (comps.size() != NumberOfScores()) {
    throw runtime_error("Size of stat vector for M2Scorer is not " + NumberOfScores());
  }
  
  float beta = beta_;
  
  //std::cerr << comps[0] << " " << comps[1] << " " << comps[2] << std::endl;
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  if(comps[1] != 0)
    p = comps[0] / (double)comps[1];
  else
    p = 1.0;
    
  if(comps[2] != 0)
    r = comps[0] / (double)comps[2];
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

float M2Scorer::getReferenceLength(const vector<ScoreStatsType>& comps) const {
  return comps[3];
}

float sentenceM2(const std::vector<ScoreStatsType>& stats)
{
  float beta = 0.5;
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  if(stats[1] != 0)
    p = stats[0] / stats[1];
  else
    p = 1.0;
    
  if(stats[2] != 0)
    r = stats[0] / stats[2];
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

}
