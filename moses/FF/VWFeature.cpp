// $Id$

#ifdef HAVE_VW

#include "VWFeature.h"
#include "moses/StaticData.h"
#include "moses/WordsRange.h"
#include "moses/Util.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <exception>

using namespace std;
using namespace Discriminative;

namespace Moses
{

VWFeature::VWFeature(const std::string &line)
  : StatelessFeatureFunction("VWFeature", 1, line)
{
  m_tgtFactors.push_back(0);
  ReadParameters();
}

void VWFeature::Load()
{
  if (m_configFile.empty())
    throw runtime_error("No config file specified for VWFeature");
  if (m_modelFile.empty())
    throw runtime_error("No model file specified for VWFeature");
  m_extractorConfig.Load(m_configFile);
  if (! m_extractorConfig.IsLoaded())
    throw runtime_error("Failed to load configuration for VWFeature");
  m_consumerFactory = new VWLibraryPredictConsumerFactory(m_modelFile, m_extractorConfig.GetVWOptionsPredict(), 255);

  m_extractor = new FeatureExtractor(m_extractorConfig, false);

  // set normalization function
  const string &normFunc = m_extractorConfig.GetNormalization();
  if (normFunc == "squared_loss") {
    m_normalizer = &NormalizeSquaredLoss;
  } else if (normFunc == "logistic_loss_basic") {
    m_normalizer = &NormalizeLogisticLossBasic;
  } else {
    throw runtime_error("Unknown normalization function: " + normFunc);
  }
}

void VWFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "config") {
    m_configFile = value;
  } else if (key == "model") {
    m_modelFile = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

ScoreComponentCollection VWFeature::ScoreFactory(float score) const
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

Translation VWFeature::GetClassifierTranslation(const TranslationOption *option) const
{
  Translation classifierOpt;

  // alignment
  const AlignmentInfo &alignInfo = option->GetTargetPhrase().GetAlignTerm();
  AlignmentInfo::const_iterator it;
  for (it = alignInfo.begin(); it != alignInfo.end(); it++)
    classifierOpt.m_alignment.insert(*it);

  // scores
  const vector<PhraseDictionary*> &ttables = StaticData::Instance().GetPhraseDictionaries();
  const ScoreComponentCollection &scoreCollection = option->GetTargetPhrase().GetScoreBreakdown();
  TTableEntry tableEntry;
  tableEntry.m_id = ""; // no domain adaptation so far
  tableEntry.m_exists = true;
  tableEntry.m_scores = scoreCollection.GetScoresForProducer(ttables[0]); // assuming one translation step!
  for (size_t i = 0; i < tableEntry.m_scores.size(); i++) {
    tableEntry.m_scores[i] = exp(tableEntry.m_scores[i]); // don't take log(log())
  }
  classifierOpt.m_ttableScores.push_back(tableEntry);

  return classifierOpt;
}

vector<ScoreComponentCollection> VWFeature::EvaluateOptions(const vector<TranslationOption *> &options, const InputType &src) const
{
  vector<ScoreComponentCollection> scores;

  if (options.size() != 0 && ! options[0]->IsOOV()) {
    vector<float> losses(options.size());
    ContextType srcContext;
    for (size_t i = 0; i < src.GetSize(); i++) {
      srcContext.push_back(GetFactors(src.GetWord(i), StaticData::Instance().GetInputFactorOrder()));
    }
    vector<Translation> classifierOptions;

    vector<TranslationOption *>::const_iterator optIt;
    for (optIt = options.begin(); optIt != options.end(); optIt++) {
      classifierOptions.push_back(GetClassifierTranslation(*optIt));
    }
    VWLibraryPredictConsumer *p_consumer = m_consumerFactory->Acquire();
    m_extractor->GenerateFeatures(p_consumer, srcContext, options[0]->GetStartPos(),
        options[0]->GetEndPos(), classifierOptions, losses);
    m_consumerFactory->Release(p_consumer);

    m_normalizer(losses); // normalize using the function specified in config file

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

void VWFeature::NormalizeSquaredLoss(vector<float> &losses)
{

	// This is (?) a good choice for sqrt loss (default loss function in VW)

  float sum = 0;

	// clip to [0,1] and take 1-Z as non-normalized prob
  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
		if (*it <= 0.0) *it = 1.0;
		else if (*it >= 1.0) *it = 0.0;
		else *it = 1.0 - *it;
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

void VWFeature::NormalizeLogisticLossBasic(vector<float> &losses)
{

	// Use this with logistic loss (we switched to this in April/May 2013)

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

void VWFeature::Normalize2(vector<float> &losses)
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

void VWFeature::Normalize3(vector<float> &losses)
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

#endif // HAVE_VW
