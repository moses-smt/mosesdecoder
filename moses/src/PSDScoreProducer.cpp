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
#include <algorithm>

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

ScoreComponentCollection PSDScoreProducer::ScoreFactory(float score)
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

vector<string> PSDScoreProducer::GetSourceFeatures(
  const InputType &srcSent,
  const Phrase *srcPhrase)
{
  vector<string> out;

  // bag of words features
  for (size_t i = 0; i < srcSent.GetSize(); i++) {
    string word = srcSent.GetWord(i).GetString(m_srcFactors, false);
    out.push_back("w^" + word);
  }

  // phrase feature
  out.push_back("p^" + Replace(srcPhrase->GetStringRep(m_srcFactors), " ", "_"));

  return out;
}

vector<string> PSDScoreProducer::GetTargetFeatures(const Phrase &tgtPhrase)
{
  vector<string> out;
  out.push_back("p^" + Replace(tgtPhrase.GetStringRep(m_tgtFactors), " ", "_"));

  return out;
}

bool PSDScoreProducer::IsOOV(const TargetPhrase &tgtPhrase)
{
  return m_phraseIndex.find(tgtPhrase.GetStringRep(m_tgtFactors)) == m_phraseIndex.end();
}

vector<ScoreComponentCollection> PSDScoreProducer::ScoreOptions(
  const vector<TranslationOption *> &options,
  const InputType &source)
{
  vector<TranslationOption *>::const_iterator it;
  vector<ScoreComponentCollection> scores;
  float sum = 0;

  if (! IsOOV(options[0]->GetTargetPhrase())) {
    vector<string> sourceFeatures = GetSourceFeatures(source, options[0]->GetSourcePhrase());

    // create VW example, add source-side features
    ezexample ex(&vwInstance.m_vw, false);
    ex(vw_namespace('s'));
    vector<string>::const_iterator fIt;
    for (fIt = sourceFeatures.begin(); fIt != sourceFeatures.end(); fIt++) {
      ex.addf(*fIt);
    }

    // get scores for all possible translations
    for (it = options.begin(); it != options.end(); it++) {
      vector<string> targetFeatures = GetTargetFeatures((*it)->GetTargetPhrase());
      string tgtPhrase = (*it)->GetTargetPhrase().GetStringRep(m_srcFactors);

      // set label to target phrase index
      ex.set_label(SPrint(m_phraseIndex[tgtPhrase]));

      // move to target namespace, add target phrase as a feature
      ex(vw_namespace('t'));
      for (fIt = targetFeatures.begin(); fIt != targetFeatures.end(); fIt++) {
        ex.addf(*fIt);
      }

      // get prediction
      float score = 1 / (1 + exp(-ex()));
      sum += score;
      scores.push_back(ScoreFactory(score));
      // move out of target namespace
      --ex;
    }
  } else {
    for (size_t i = 0; i < options.size(); i++) {
      scores.push_back(ScoreFactory(0));
    }
  }

  // normalize
  if (sum != 0) {
    vector<ScoreComponentCollection>::iterator colIt;
    for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
      colIt->Assign(this, log(colIt->GetScoreForProducer(this) / sum));
    }
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
  while (getline(in, line)) {
    vector<string> columns = Tokenize(line, "\t");
    size_t idx = Scan<size_t>(columns[1]);
    m_phraseIndex.insert(make_pair<string, size_t>(columns[0], idx));
  }
  in.close();

  return true;
}

} // namespace Moses
