// $Id$

#ifndef moses_PSDScoreProducer_h
#define moses_PSDScoreProducer_h

#include "FeatureFunction.h"
#include "TypeDef.h"
#include "TranslationOption.h"
#include "ScoreComponentCollection.h"
#include "InputType.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace Moses
{

class PSDScoreProducer : public StatelessFeatureFunction
{
public:
  PSDScoreProducer(ScoreIndexManager &scoreIndexManager, float weight);

  // read required data files
  bool Initialize(const std::string &modelFile, const std::string &indexFile);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  std::vector<ScoreComponentCollection> ScoreOptions(const std::vector<TranslationOption *> &options, const InputType &src);

  // mandatory methods for features
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  virtual bool ComputeValueInTranslationOption() const
  {
    return true;
  }
private:
  bool LoadPhraseIndex(const std::string &indexFile);
  bool IsOOV(const TargetPhrase &tgtPhrase);
  ScoreComponentCollection ScoreFactory(float score);

  std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now
  PSD::TargetIndexType m_phraseIndex;
  PSD::FeatureConsumer *m_consumer;
  PSD::FeatureExtractor *m_extractor;
  std::ifstream m_contextFile;
};

}

#endif // moses_PSDScoreProducer_h
