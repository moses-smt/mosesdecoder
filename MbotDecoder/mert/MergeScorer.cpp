#include "MergeScorer.h"

#include <cmath>
#include <stdexcept>
#include "ScoreStats.h"
#include "TerScorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"
#include "CderScorer.h"

#include "TERsrc/tercalc.h"
#include "TERsrc/terAlignment.h"

using namespace TERCpp;

MergeScorer::MergeScorer(const string& config)
    : StatisticsBasedScorer("MERGE",config), kLENGTH(4) {}
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
  float result=0.0;
  float weight=1.0;
  float resultTmp=0.0;
  vector<int> vecLine;
  vector<string> vecScorerType;
  vector<float> weightsModifier;
  int pos=0;
  int weightIncrement=0;
  string initfile="merge.init";
  string line;
  ifstream opt(initfile.c_str());
  float denom=0.0;
  if (opt.fail()) {
    cerr<<"MergeScorer::calculateScore : could not open initfile: " << initfile << endl;
    exit(3);
  }
  while (getline (opt, line)) {
    vector<string> vecLine=stringToVector(line, " ");
    if ((int)vecLine.size() != 4) {
      cerr<<"MergeScorer::calculateScore : Error in initfile: " << initfile << endl;
      exit(4);
    }
    vecScorerType.push_back(vecLine.at(0));
    weightsModifier.push_back(atof(vecLine.at(1).c_str()));
    denom+=abs(atof(vecLine.at(1).c_str()));
  }
  for (weightIncrement = 0; weightIncrement < (int) weightsModifier.size(); weightIncrement++)
  {
    if (vecScorerType.at(weightIncrement).compare("BLEU")==0)
    {
      BleuScorer* scorer01= new BleuScorer("");
      weight=weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      vecLine=subVector(comps, pos,pos+(int)(scorer01->NumberOfScores()));
      pos=pos+(int)(scorer01->NumberOfScores());
      resultTmp=(scorer01->calculateScore(vecLine));
      result+=(weight * resultTmp);

    }
    else if (vecScorerType.at(weightIncrement).compare("TER")==0)
    {
      TerScorer* scorer02= new TerScorer("");
      weight=weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      vecLine=subVector(comps, pos,pos+(int)(scorer02->NumberOfScores()));
      pos=pos+(int)(scorer02->NumberOfScores());
      resultTmp=(scorer02->calculateScore(vecLine));
      result+=(weight * resultTmp);
    }
    else if (vecScorerType.at(weightIncrement).compare("PER")==0)
    {
      PerScorer* scorer03= new PerScorer("");
      weight=weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      vecLine=subVector(comps, pos,pos+(int)(scorer03->NumberOfScores()));
      pos=pos+(int)(scorer03->NumberOfScores());
      resultTmp=(scorer03->calculateScore(vecLine));
      result+=(weight * result);
    }
    else if (vecScorerType.at(weightIncrement).compare("CER")==0)
    {
      CderScorer* scorer04= new CderScorer("");
      weight=weightsModifier.at(weightIncrement) / denom;
      vecLine.clear();
      vecLine=subVector(comps, pos,pos+(int)(scorer04->NumberOfScores()));
      pos=pos+(int)(scorer04->NumberOfScores());
      resultTmp=(scorer04->calculateScore(vecLine));
      result+=(weight * resultTmp);
    }
    else
    {
      throw runtime_error("MergeScorer::calculateScore : Scorer unknown");
      exit(0);
    }
  }
  return result;
}
