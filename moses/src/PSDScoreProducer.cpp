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
  m_srcFactors.push_back(0); 
  m_srcFactors.push_back(1);
  m_srcFactors.push_back(2);

  m_tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

bool PSDScoreProducer::Initialize(const string &modelFile, const string &indexFile)
{
  m_consumer = new VWLibraryPredictConsumer(modelFile);
  if (! LoadPhraseIndex(indexFile))
    return false;

  m_extractor = new FeatureExtractor(m_phraseIndex, false);
  return true;
}

ScoreComponentCollection PSDScoreProducer::ScoreFactory(float score)
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

bool PSDScoreProducer::IsOOV(const TargetPhrase &tgtPhrase)
{
  return m_phraseIndex.left.find(tgtPhrase.GetStringRep(m_tgtFactors)) == m_phraseIndex.left.end();
}

void PSDScoreProducer::SetSentence(const InputType &inputSent)
{
  m_currentContext.clear();
  for (size_t i = 0; i < inputSent.GetSize(); i++) {
    vector<string> factors;
    Word word = inputSent.GetWord(i);
    vector<size_t>::const_iterator it;
    for (it = m_srcFactors.begin(); it != m_srcFactors.end(); it++) {
      factors.push_back(word.GetFactor(*it)->GetString());
    }
    m_currentContext.push_back(factors);
  }
}

vector<ScoreComponentCollection> PSDScoreProducer::ScoreOptions(const vector<TranslationOption *> &options)
{
  vector<ScoreComponentCollection> scores;
  float sum = 0;

  if (options.size() != 0 && ! IsOOV(options[0]->GetTargetPhrase())) {
    vector<float> losses(options.size());
    vector<size_t> optionIDs(options.size());

    vector<TranslationOption *>::const_iterator optIt;
    for (optIt = options.begin(); optIt != options.end(); optIt++) {
      string tgtPhrase = (*optIt)->GetTargetPhrase().GetStringRep(m_srcFactors);
      optionIDs.push_back(m_phraseIndex.left.find(tgtPhrase)->second);
    }
    m_extractor->GenerateFeatures(m_consumer, m_currentContext, options[0]->GetStartPos(),
        options[0]->GetEndPos(), optionIDs, losses);

    vector<float>::iterator lossIt;
    for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
      *lossIt = exp(-*lossIt);
      sum += *lossIt;
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
    m_phraseIndex.insert(TargetIndexType::value_type(columns[0], idx));
  }
  in.close();

  return true;
}

} // namespace Moses
