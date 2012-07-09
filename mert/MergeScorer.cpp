#include "MergeScorer.h"

#include <cmath>
#include <stdexcept>
#include "ScoreStats.h"
#include "TerScorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"
#include "CderScorer.h"

#include "TER/tercalc.h"
#include "TER/terAlignment.h"

using namespace std;
using namespace TERCpp;

namespace MosesTuning
{
  

MergeScorer::MergeScorer(const string& config)
    : StatisticsBasedScorer("MERGE", config) {}

MergeScorer::~MergeScorer() {}

void MergeScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  throw runtime_error("MERGE Scorer can be used only in mert execution");
  exit(0);
}

void MergeScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  throw runtime_error("MergeScorer::prepareStats : MERGE Scorer can be used only in mert execution");
  exit(0);
}

/*
float MergeScorer::calculateScore(const vector<int>& comps)
{
        throw runtime_error("MergeScorer::calculateScore : MERGE Scorer can be used only in mert execution");
    exit(0);
}
*/

float MergeScorer::calculateScore(const std::vector< int >& comps) const
{
  float result = 0.0;
  vector<int> vecLine;
  vector<string> vecScorerType;
  vector<float> weightsModifier;
  int pos = 0;
  int weightIncrement = 0;
  string initfile = "merge.init";
  string line;
  ifstream opt(initfile.c_str());
  float denom = 0.0;

  if (opt.fail()) {
    cerr<<"MergeScorer::calculateScore : could not open initfile: " << initfile << endl;
    exit(3);
  }
  while (getline (opt, line)) {
    vector<string> vecLine=stringToVector(line, " ");
    if (vecLine.size() != 4) {
      cerr<<"MergeScorer::calculateScore : Error in initfile: " << initfile << endl;
      exit(4);
    }
    vecScorerType.push_back(vecLine.at(0));
    weightsModifier.push_back(atof(vecLine.at(1).c_str()));
    denom += abs(atof(vecLine.at(1).c_str()));
  }
  const int weights_modifier_size = static_cast<int>(weightsModifier.size());
  for (weightIncrement = 0; weightIncrement < weights_modifier_size; weightIncrement++)
  {
    if (vecScorerType.at(weightIncrement).compare("BLEU") == 0)
    {
      BleuScorer* scorer01 = new BleuScorer("");
      const float weight = weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      const int num_scores = static_cast<int>(scorer01->NumberOfScores());
      vecLine = subVector(comps, pos, pos + num_scores);
      pos += num_scores;
      result += weight * scorer01->calculateScore(vecLine);;
      delete scorer01;
    }
    else if (vecScorerType.at(weightIncrement).compare("TER") == 0)
    {
      TerScorer* scorer02 = new TerScorer("");
      const float weight = weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      const int num_scores = static_cast<int>(scorer02->NumberOfScores());
      vecLine = subVector(comps, pos, pos + num_scores);
      pos += num_scores;
      result += weight * scorer02->calculateScore(vecLine);
      delete scorer02;
    }
    else if (vecScorerType.at(weightIncrement).compare("PER") == 0)
    {
      PerScorer* scorer03 = new PerScorer("");
      const float weight = weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      const int num_scores = static_cast<int>(scorer03->NumberOfScores());
      vecLine = subVector(comps, pos, pos + num_scores);
      pos += num_scores;
      result += weight * scorer03->calculateScore(vecLine);
      delete scorer03;
    }
    else if (vecScorerType.at(weightIncrement).compare("CER") == 0)
    {
      CderScorer* scorer04 = new CderScorer("");
      const float weight = weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      const int num_scores = static_cast<int>(scorer04->NumberOfScores());
      vecLine = subVector(comps, pos, pos + num_scores);
      pos += num_scores;
      result += weight * scorer04->calculateScore(vecLine);
      delete scorer04;
    }
    else
    {
      throw runtime_error("MergeScorer::calculateScore : Scorer unknown");
      exit(0);
    }
  }
  return result;
}

}

