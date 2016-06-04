#include "M2Scorer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>

#include <boost/lexical_cast.hpp>


using namespace std;

namespace MosesTuning
{

M2Scorer::M2Scorer(const string& config)
  : StatisticsBasedScorer("M2Scorer", config),
    beta_(Scan<float>(getConfig("beta", "0.5"))),
    max_unchanged_words_(Scan<int>(getConfig("max_unchanged_words", "2"))),
    truecase_(Scan<bool>(getConfig("truecase", "false"))),
    verbose_(Scan<bool>(getConfig("verbose", "false"))),
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
  std::vector<ScoreStatsType> stats(4, 0);
  m2_.SufStats(sentence, sid, stats);
  entry.set(stats);
}

float M2Scorer::calculateScore(const vector<ScoreStatsType>& comps) const
{

  if (comps.size() != NumberOfScores()) {
    throw runtime_error("Size of stat vector for M2Scorer is not " + NumberOfScores());
  }

  float beta = beta_;


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

  if(verbose_)
    std::cerr << comps[0] << " " << comps[1] << " " << comps[2] << std::endl;

  if(verbose_)
    std::cerr << p << " " << r << " " << f << std::endl;

  return f;
}

float M2Scorer::getReferenceLength(const vector<ScoreStatsType>& comps) const
{
  return comps[3];
}

std::vector<ScoreStatsType> randomStats(float decay, int max)
{
  int gold = rand() % max;
  int prop = rand() % max;
  int corr = 0.0;

  if(std::min(prop, gold) > 0)
    corr = rand() % std::min(prop, gold);

  //std::cerr << corr << " " << prop << " " << gold << std::endl;

  std::vector<ScoreStatsType> stats(3, 0.0);
  stats[0] = corr * decay;
  stats[1] = prop * decay;
  stats[2] = gold * decay;

  return stats;
}

float sentenceM2(const std::vector<ScoreStatsType>& stats)
{
  float beta = 0.5;

  std::vector<ScoreStatsType> smoothStats(3, 0.0); // = randomStats(0.001, 5);
  smoothStats[0] += stats[0];
  smoothStats[1] += stats[1];
  smoothStats[2] += stats[2];

  float p = 0.0;
  float r = 0.0;
  float f = 0.0;

  if(smoothStats[1] != 0)
    p = smoothStats[0] / smoothStats[1];
  else
    p = 1.0;

  if(smoothStats[2] != 0)
    r = smoothStats[0] / smoothStats[2];
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
