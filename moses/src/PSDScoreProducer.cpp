// $Id$

#include "util/check.hh"
#include "vw.h"
#include "ezexample.h"
#include "FFState.h"
#include "StaticData.h"
#include "PSDScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
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

const FFState* PSDScoreProducer::EmptyHypothesisState(const InputType &input) const
{
  return NULL; // don't need previous states
}

feature PSDScoreProducer::feature_from_string(const string &feature_str, unsigned long seed, float value)
{
  uint32_t feature_hash = VW::hash_feature(vwInstance.m_vw, feature_str, seed);
  feature f = { value, feature_hash };
  return f;
}

void PSDScoreProducer::Initialize(const string &modelFile)
{
  vwInstance.m_vw = VW::initialize("--hash all -q st --noconstant -i " + modelFile);
}

FFState* PSDScoreProducer::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* out) const
{

  // get the source and target phrase
  string srcPhrase = hypo.GetSourcePhraseStringRep(m_srcFactors);
  string tgtPhrase = hypo.GetTargetPhraseStringRep(m_tgtFactors);

  ezexample ex(& vwInstance.m_vw, false);
  ex(vw_namespace('s')) ("p^" + srcPhrase);

  for (size_t i = 0; i < hypo.GetInput().GetSize(); i++) {
    string word = hypo.GetInput().GetWord(i).GetString(m_srcFactors, false);
    ex("w^" + word);
  }

  // TODO go over all target phrases
  ex(vw_namespace('t')) ("p^" + tgtPhrase);

  out->PlusEquals(this, log(ex())); 
  
  return NULL;
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

} // namespace Moses
