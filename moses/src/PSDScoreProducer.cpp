// $Id$

#include "util/check.hh"
#include "vw.h"
#include "ezexample.h"
#include "FFState.h"
#include "StaticData.h"
#include "PSDScoreProducer.h"
#include "WordsRange.h"
#include "Util.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{

VWInstance vwInstance;

PSDScoreProducer::PSDScoreProducer(ScoreIndexManager &scoreIndexManager, float weight)
{
  scoreIndexManager.AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);
  m_srcFactors.push_back(0);
  m_tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

bool PSDScoreProducer::Initialize(const string &modelFile, const string &indexFile)
{
  vwInstance.m_vw = VW::initialize("--hash all -q st --noconstant -i " + modelFile);
  return LoadPhraseIndex(indexFile);
}

vector<ScoreComponentCollection> PSDScoreProducer::ScoreOptions(
  const vector<TranslationOption *> &options,
  const InputType &source)
{
  vector<TranslationOption *>::const_iterator it;
  vector<ScoreComponentCollection> scores;
  float sum = 0;

  string srcPhrase = (*it)->GetSourcePhrase()->GetStringRep(m_srcFactors);

  // create VW example, add source-side features
  ezexample ex(&vwInstance.m_vw, false);
  ex(vw_namespace('s')) ("p^" + Replace(srcPhrase, " ", "_"));

  for (size_t i = 0; i < source.GetSize(); i++) {
    string word = source.GetWord(i).GetString(m_srcFactors, false);
    ex("w^" + word);
  }

  // get scores for all possible translations
  for (it = options.begin(); it != options.end(); it++) {
    string tgtPhrase = (*it)->GetTargetPhrase().GetStringRep(m_srcFactors);

    // set label to target phrase index
    ex.set_label(SPrint(m_phraseIndex[tgtPhrase]));

    // move to target namespace, add target phrase as a feature
    ex(vw_namespace('t')) ("p^" + Replace(tgtPhrase, " ", "_"));

    // get prediction
    float score = ex();
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
    colIt->Assign(this, colIt->GetScoreForProducer(this) / sum);
  }

  return scores;
}

size_t PSDScoreProducer::GetNumScoreComponents() const
{
  return 1;
}

std::string PSDScoreProducer::GetScoreProducerDescription(unsigned) const
{
  return "PSD";
}

std::string PSDScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "psd";
}

size_t PSDScoreProducer::GetNumInputScores() const
{
  return 0;
}

bool PSDScoreProducer::LoadPhraseIndex(const string &indexFile)
{
  ifstream in(indexFile.c_str());
  if (!in.good())
    return false;
  string line;
  while (in) {
    size_t idx;
    string phrase;
    in >> phrase >> idx;
    m_phraseIndex.insert(make_pair<string, size_t>(phrase, idx));
  }
  in.close();

  return true;
}

} // namespace Moses
