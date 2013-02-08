// $Id$

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
  TTableEntry tableEntry;
  tableEntry.m_id = ""; // no domain adaptation so far
  tableEntry.m_exists = true;
  tableEntry.m_scores = scoreCollection.GetScoresForProducer(ttables[0]); // assuming one translation step!
  for (size_t i = 0; i < tableEntry.m_scores.size(); i++) {
    tableEntry.m_scores[i] = exp(tableEntry.m_scores[i]); // don't take log(log())
  }
  psdOpt.m_ttableScores.push_back(tableEntry);

  return psdOpt;
}

vector<ScoreComponentCollection> PSDScoreProducer::ScoreOptions(const vector<TranslationOption *> &options, const InputType &src)
{
  vector<ScoreComponentCollection> scores;

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

    Normalize0(losses);

    vector<float>::const_iterator lossIt;
    for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
      float logScore = Equals(*lossIt, 0) ? LOWEST_SCORE : log(*lossIt);
      scores.push_back(ScoreFactory(logScore));
    }
  } else {
    for (size_t i = 0; i < options.size(); i++)
      scores.push_back(ScoreFactory(0));
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

void PSDScoreProducer::Normalize0(vector<float> &losses)
{
  float sum = 0;

	// clip to [0,1] and take 1-Z as non-normalized prob
  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
		if (*it <= 0.0) *it = 1.0;
		else if (*it >= 1.0) *it = 0.0;
		else *it = 1 - *it;
		sum += *it;
  }

  if (! Equals(sum, 0)) {
		// normalize
    for (it = losses.begin(); it != losses.end(); it++)
      *it /= sum;
  } else {
		// sum of non-normalized probs is 0, then take uniform probs
    for (it = losses.begin(); it != losses.end(); it++) 
      *it = 1.0 / losses.size();
  }
}

void PSDScoreProducer::Normalize1(vector<float> &losses)
{
  float sum = 0;
  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
    *it = exp(-*it);
    sum += *it;
  }
  for (it = losses.begin(); it != losses.end(); it++) {
    *it /= sum;
  }
}

void PSDScoreProducer::Normalize2(vector<float> &losses)
{
  float sum = 0;
  float minLoss;
  if (losses.size() > 0)
    minLoss = -losses[0];

  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
    *it = -*it;
    minLoss = min(minLoss, *it);
  }

  for (it = losses.begin(); it != losses.end(); it++) {
    *it -= minLoss;
    sum += *it;
  }

  if (! Equals(sum, 0)) {
    for (it = losses.begin(); it != losses.end(); it++)
      *it /= sum;
  } else {
    for (it = losses.begin(); it != losses.end(); it++) 
      *it = 1.0 / losses.size();
  }
}

void PSDScoreProducer::Normalize3(vector<float> &losses)
{
  float sum = 0;
  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
		// Alex changed this to be *it rather than -*it because it sorted backwards; not sure if this is right though!
    //*it = 1 / (1 + exp(-*it));
    *it = 1 / (1 + exp(*it));
    sum += *it;
  }
  for (it = losses.begin(); it != losses.end(); it++)
    *it /= sum;
}

} // namespace Moses
