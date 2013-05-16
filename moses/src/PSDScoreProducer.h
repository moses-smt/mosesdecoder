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
  bool Initialize(const std::string &modelFile, const std::string &indexFile, const std::string &configFile);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  std::vector<ScoreComponentCollection> ScoreOptions(const std::vector<TranslationOption *> &options, const InputType &src);

  // mandatory methods for Moses feature functions
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  // calculate scores when collecting translation options, not during decoding
  virtual bool ComputeValueInTranslationOption() const
  {
    return true;
  }
private:
  // Load index/vocabulary of target phrases
  bool LoadPhraseIndex(const std::string &indexFile);

  // throw an exception if target phrase is not in phrase vocabulary
  void CheckIndex(const TargetPhrase &tgtPhrase);

  // Construct a ScoreComponentCollection with PSD feature set to given score
  ScoreComponentCollection ScoreFactory(float score);

  // Build PSD::Translation object from Moses structures
  PSD::Translation GetPSDTranslation(const TranslationOption *option);

  static void NormalizeSquaredLoss(std::vector<float> &losses);
  static void NormalizeLogisticLossBasic(std::vector<float> &losses);
  static void Normalize2(std::vector<float> &losses);
  static void Normalize3(std::vector<float> &losses);

  std::vector<FactorType> m_tgtFactors; // which factors to use; XXX hard-coded for now
  PSD::IndexType m_phraseIndex;
  PSD::VWLibraryPredictConsumerFactory	*m_consumerFactory;
  PSD::FeatureExtractor *m_extractor;
  PSD::ExtractorConfig m_extractorConfig;
  std::ifstream m_contextFile;
  void (*m_normalizer)(std::vector<float> &); // normalization function
};

}

#endif // moses_PSDScoreProducer_h
