// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "CellContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include "vw.h"
#include "ezexample.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{

VWInstance vwInstance;

CellContextScoreProducer::CellContextScoreProducer(ScoreIndexManager &scoreIndexManager, float weight)
{
  scoreIndexManager.AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);
  m_srcFactors.push_back(0);
  m_tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

bool CellContextScoreProducer::Initialize(const string &modelFile, const string &indexFile)
{
  vwInstance.m_vw = VW::initialize("--hash all -q st --noconstant -i " + modelFile);
  return LoadRuleIndex(indexFile);
}

bool CellContextScoreProducer::LoadRuleIndex(const string &indexFile)
{
  ifstream in(indexFile.c_str());
  if (!in.good())
    return false;
  string line;
  while (getline(in, line)) {
    vector<string> columns = Tokenize(line, "\t");
    size_t idx = Scan<size_t>(columns[1]);
    m_ruleIndex.insert(make_pair<string, size_t>(columns[0], idx));
  }
  in.close();
  return true;
}

vector<ScoreComponentCollection> CellContextScoreProducer::ScoreRules(
                                                                      const std::string &sourceSide,
                                                                      std::vector<std::string> * targetRepresentations,
                                                                      const InputType source
                                                                      )
{
  vector<ScoreComponentCollection> scores;
  float sum = 0;

  // create VW example, add source-side features
  ezexample ex(&vwInstance.m_vw, false);
  ex(vw_namespace('s')) ("p^" + Replace(sourceSide, " ", "_"));

  for (size_t i = 0; i < source.GetSize(); i++) {
    string word = source.GetWord(i).GetString(m_srcFactors, false);
    ex("w^" + word);
  }

   std::vector<std::string> itr_targ_rep;
  // get scores for all possible translations
  for (itr_targ_rep = targetRepresentations->begin(); itr_targ_rep != targetRepresentations->end(); itr_targ_rep++) {
    string tgtRep = *itr_targ_rep;

    // set label to target phrase index
    ex.set_label(SPrint(m_phraseIndex[tgtRep]));

    // move to target namespace, add target phrase as a feature
    ex(vw_namespace('t')) ("p^" + Replace(tgtRep, " ", "_"));

    // get prediction
    float score = 1 / (1 + exp(-ex()));

//    cerr << srcPhrase << " ||| " << tgtPhrase << " ||| " << score << endl;

    sum += score;

    // create score object
    ScoreComponentCollection scoreCol;
    scoreCol.Assign(this, score);
    scores.push_back(scoreCol);

    // move out of target namespace
    --ex;
  }
  VW::finish(vwInstance.m_vw);

  // normalize
  vector<ScoreComponentCollection>::iterator colIt;
  for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
    colIt->Assign(this, log(colIt->GetScoreForProducer(this) / sum));
  }

  return scores;
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

size_t CellContextScoreProducer::GetNumInputScores() const
{
  return 0;
}

} // namespace Moses
