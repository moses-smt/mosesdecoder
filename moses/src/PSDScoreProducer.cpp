// $Id: PSDScoreProducer.cpp,v 1.1 2012/10/07 13:43:03 braunefe Exp $

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "PSDScoreProducer.h"
#include "WordsRange.h"
#include "Util.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <exception>

using namespace std;
using namespace boost::bimaps;
using namespace PSD;

namespace Moses
{

PSDScoreProducer::PSDScoreProducer(ScoreIndexManager &scoreIndexManager, float weight)
{
  scoreIndexManager.AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);

  m_tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

bool PSDScoreProducer::Initialize(const string &modelFile, const string &indexFile, const string &configFile)
{
  m_consumerFactory = new VWLibraryPredictConsumerFactory(modelFile, 255);
  if (! LoadPhraseIndex(indexFile))
    return false;

  m_extractorConfig.Load(configFile);

  m_extractor = new FeatureExtractor(m_phraseIndex, m_extractorConfig, false);
  return true;
}

ScoreComponentCollection PSDScoreProducer::ScoreFactory(float score)
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

void PSDScoreProducer::CheckIndex(const TargetPhrase &tgtPhrase)
{
  string phraseStr = tgtPhrase.GetStringRep(m_tgtFactors);
  if (m_phraseIndex.left.find(phraseStr) == m_phraseIndex.left.end())
    throw runtime_error("Phrase not in index: " + phraseStr);
}

Translation PSDScoreProducer::GetPSDTranslation(const TranslationOption *option)
{
  Translation psdOpt;

  // phrase ID
  string tgtPhrase = option->GetTargetPhrase().GetStringRep(m_tgtFactors);
  psdOpt.m_index = m_phraseIndex.left.find(tgtPhrase)->second;

  // alignment
  const AlignmentInfo &alignInfo = option->GetTargetPhrase().GetAlignmentInfo();
  AlignmentInfo::const_iterator it;
  for (it = alignInfo.begin(); it != alignInfo.end(); it++)
    psdOpt.m_alignment.insert(*it);

  // scores
  const TranslationSystem& system = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
  const vector<PhraseDictionaryFeature*>& ttables = system.GetPhraseDictionaries();
  const ScoreComponentCollection &scoreCollection = option->GetTargetPhrase().GetScoreBreakdown();
  psdOpt.m_scores = scoreCollection.GetScoresForProducer(ttables[0]); // assuming one translation step!

  return psdOpt;
}

vector<ScoreComponentCollection> PSDScoreProducer::ScoreOptions(const vector<TranslationOption *> &options, const InputType &src)
{
  vector<ScoreComponentCollection> scores;
  float sum = 0;

  if (options.size() != 0 && ! options[0]->IsOOV()) {
    vector<float> losses(options.size());
    vector<Translation> psdOptions;

    vector<TranslationOption *>::const_iterator optIt;
    for (optIt = options.begin(); optIt != options.end(); optIt++) {
      CheckIndex((*optIt)->GetTargetPhrase());
      psdOptions.push_back(GetPSDTranslation(*optIt));
    }
    VWLibraryPredictConsumer * p_consumer = m_consumerFactory->Acquire();
    m_extractor->GenerateFeatures(p_consumer, src.m_PSDContext, options[0]->GetStartPos(),
        options[0]->GetEndPos(), psdOptions, losses);
    m_consumerFactory->Release(p_consumer);

    vector<float>::iterator lossIt;
    for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
      float score = exp(-*lossIt);
      sum += score;
      scores.push_back(ScoreFactory(score));
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
  size_t index = 0;
  while (getline(in, line)) {
    m_phraseIndex.insert(TargetIndexType::value_type(line, ++index));
  }
  in.close();

  return true;
}

} // namespace Moses
