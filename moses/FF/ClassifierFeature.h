// $Id$

#ifndef moses_ClassifierFeature_h
#define moses_ClassifierFeature_h

#ifdef HAVE_VW

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

class ClassifierFeature : public StatelessFeatureFunction
{
public:
  ClassifierFeature(ScoreIndexManager &scoreIndexManager, float weight);

  // read required data files
  bool Initialize(const std::string &modelFile, const std::string &configFile);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  std::vector<ScoreComponentCollection> EvaluateOptions(const std::vector<TranslationOption *> &options, const InputType &src) const;

  // mandatory methods for Moses feature functions
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  // calculate scores when collecting translation options, not during decoding
  virtual bool ComputedInTranslationOption() const
  {
    return true;
  }
private:
  // Construct a ScoreComponentCollection with Classifier feature set to given score
  ScoreComponentCollection ScoreFactory(float score) const;

  // Build Classifier::Translation object from Moses structures
  Classifier::Translation GetClassifierTranslation(const TranslationOption *option) const;

  static void NormalizeSquaredLoss(std::vector<float> &losses) const;
  static void NormalizeLogisticLossBasic(std::vector<float> &losses) const;
  static void Normalize2(std::vector<float> &losses) const;
  static void Normalize3(std::vector<float> &losses) const;

  std::vector<FactorType> m_tgtFactors; // which factors to use; XXX hard-coded for now
  mutable Classifier::VWLibraryPredictConsumerFactory	*m_consumerFactory; // XXX mutable
  mutable Classifier::FeatureExtractor *m_extractor; // XXX mutable
  Classifier::ExtractorConfig m_extractorConfig;
  void (*m_normalizer)(std::vector<float> & const); // normalization function
};

}

#endif // HAVE_VW

#endif // moses_ClassifierFeature_h
