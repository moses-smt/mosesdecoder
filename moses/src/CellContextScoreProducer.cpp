// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "Classifier.h"
#include "CellContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{

CellContextScoreProducer::CellContextScoreProducer(float weight)
{
  const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

void CellContextScoreProducer::LoadScores(const string &ttableFile)
{
  //Classifier::Instance().LoadScores(ttableFile);
}

void CellContextScoreProducer::EvaluateChart(const TargetPhrase&, ScoreComponentCollection* out, float score) const
{
  out->PlusEquals(this, score);
}

size_t CellContextScoreProducer::GetNumScoreComponents() const
{
  return 1; // let's return just P(e|f) for now
}

std::string CellContextScoreProducer::GetScoreProducerDescription(unsigned) const
{
  return "CellContext";
}

std::string CellContextScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "cc";
}

  /*size_t CellContextScoreProducer::GetNumInputScores() const
{
  return 0;
  }*/

} // namespace Moses
