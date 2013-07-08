// $Id$

#ifndef moses_ClassifierFeature_h
#define moses_ClassifierFeature_h

#ifdef HAVE_VW

#include "FeatureFunction.h"
#include "moses/TypeDef.h"
#include "moses/TranslationOption.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/InputType.h"
#include "contrib/classifier/FeatureExtractor.h"
#include "contrib/classifier/FeatureConsumer.h"

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
  ClassifierFeature(const std::string &line);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  std::vector<ScoreComponentCollection> EvaluateOptions(const std::vector<TranslationOption *> &options, const InputType &src) const;

  // methods for Moses feature functions
  void Load();
  void SetParameter(const std::string& key, const std::string& value);
  bool IsUseable(const FactorMask &mask) const { return true; }

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

  static void NormalizeSquaredLoss(std::vector<float> &losses);
  static void NormalizeLogisticLossBasic(std::vector<float> &losses);
  static void Normalize2(std::vector<float> &losses);
  static void Normalize3(std::vector<float> &losses);

  std::vector<std::string> GetFactors(const Word &word, const std::vector<FactorType> &factors) {
    std::vector<std::string> out;
    std::vector<FactorType>::const_iterator it = factors.begin();
    while (it != factors.end()) {
      out.push_back(word.GetFactor(*it)->GetString().as_string());
    }
    return out;
  }

  std::vector<FactorType> m_tgtFactors; // which factors to use; XXX hard-coded for now
  mutable Classifier::VWLibraryPredictConsumerFactory	*m_consumerFactory; // XXX mutable
  mutable Classifier::FeatureExtractor *m_extractor; // XXX mutable
  Classifier::ExtractorConfig m_extractorConfig;
  void (*m_normalizer)(std::vector<float> &); // normalization function
  std::string m_configFile, m_modelFile;
};

}

#endif // HAVE_VW

#endif // moses_ClassifierFeature_h
