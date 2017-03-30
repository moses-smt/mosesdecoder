#include "QEScorer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>

#include <boost/lexical_cast.hpp>


using namespace std;

namespace MosesTuning
{

QEScorer::QEScorer(const string& config)
  : StatisticsBasedScorer("QE", config), bad_(false)
{
   const std::string type = getConfig("type", "mult");
   if(type == "bad")
     bad_ = true;
}

void QEScorer::setReferenceFiles(const vector<string>& referenceFiles)
{}

void QEScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{}

float QEScorer::calculateScore(const vector<ScoreStatsType>& comps) const
{

  if (comps.size() != NumberOfScores()) {
    throw runtime_error("Size of stat vector for QE is not " + NumberOfScores());
  }

  float p1 = 0.0;
  float r1 = 0.0;
  float f1 = 0.0;

  float p2 = 0.0;
  float r2 = 0.0;
  float f2 = 0.0;

  if(comps[0] != 0)
    p1 = (float)comps[0] / (comps[0] + comps[1]);
  else
    p1 = 1.0;

  if(comps[2] != 0)
    r1 = (float)comps[0] / (comps[0] + comps[2]);
  else
    r1 = 1.0;

  float denom1 = p1 + r1;
  if(denom1 != 0)
    f1 = 2 * p1 * r1 / denom1;
  else
    f1 = 0.0;

  if(comps[3] != 0)
    p2 = (float)comps[3] / (comps[3] + comps[1]);
  else
    p2 = 1.0;

  if(comps[2] != 0)
    r2 = (float)comps[3] / (comps[3] + comps[2]);
  else
    r2 = 1.0;

  float denom2 = p2 + r2;
  if(denom2 != 0)
    f2 = 2 * p2 * r2 / denom2;
  else
    f2 = 0.0;

  if(bad_)
    return f1;
  return f1 * f2;
}

}
