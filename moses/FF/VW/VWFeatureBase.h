#pragma once

#include <string>
#include <boost/thread/tss.hpp>

#include "vw/Classifier.h"
#include "moses/TypeDef.h"
#include "moses/TranslationTask.h"
#include "moses/Util.h"
#include "moses/FF/StatelessFeatureFunction.h"

namespace Moses
{

enum VWFeatureType {
  vwft_source,
  vwft_target,
  vwft_targetContext
};

class VWFeatureBase : public StatelessFeatureFunction
{
public:
  VWFeatureBase(const std::string &line, VWFeatureType featureType = vwft_source)
    : StatelessFeatureFunction(0, line), m_usedBy(1, "VW0"), m_featureType(featureType) {
    // defaults
    m_sourceFactors.push_back(0);
    m_targetFactors.push_back(0);
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  // Official hooks should do nothing. This is a hack to be able to define
  // classifier features in the moses.ini configuration file.
  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {}
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {}
  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {}
  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {}
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {}


  // Common parameters for classifier features, both source and target features
  virtual void SetParameter(const std::string& key, const std::string& value) {
    if (key == "used-by") {
      ParseUsedBy(value);
    } else if (key == "source-factors") {
      Tokenize<FactorType>(m_sourceFactors, value, ",");
    } else if (key == "target-factors") {
      Tokenize<FactorType>(m_targetFactors, value, ",");
    } else {
      StatelessFeatureFunction::SetParameter(key, value);
    }
  }

  // Return all classifier features, regardless of type
  static const std::vector<VWFeatureBase*>& GetFeatures(std::string name = "VW0") {
    UTIL_THROW_IF2(s_features.count(name) == 0, "No features registered for parent classifier: " + name);
    return s_features[name];
  }

  // Return only source-dependent classifier features
  static const std::vector<VWFeatureBase*>& GetSourceFeatures(std::string name = "VW0") {
    UTIL_THROW_IF2(s_sourceFeatures.count(name) == 0, "No source features registered for parent classifier: " + name);
    return s_sourceFeatures[name];
  }

  // Return only target-context classifier features
  static const std::vector<VWFeatureBase*>& GetTargetContextFeatures(std::string name = "VW0") {
    // don't throw an exception when there are no target-context features, this feature type is not mandatory
    return s_targetContextFeatures[name];
  }

  // Return only target-dependent classifier features
  static const std::vector<VWFeatureBase*>& GetTargetFeatures(std::string name = "VW0") {
    UTIL_THROW_IF2(s_targetFeatures.count(name) == 0, "No target features registered for parent classifier: " + name);
    return s_targetFeatures[name];
  }

  // Required length context (maximum context size of defined target-context features)
  static size_t GetMaximumContextSize(std::string name = "VW0") {
    return s_targetContextLength[name]; // 0 by default
  }

  // Overload to process source-dependent data, create features once for every
  // source sentence word range.
  virtual void operator()(const InputType &input
                          , const Range &sourceRange
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const = 0;

  // Overload to process target-dependent features, create features once for
  // every target phrase. One source word range will have at least one target
  // phrase, but may have more.
  virtual void operator()(const InputType &input
                          , const TargetPhrase &targetPhrase
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const = 0;

  // Overload to process target-context dependent features, these features are
  // evaluated during decoding. For efficiency, features are not fed directly into
  // the classifier object but instead output in the vector "features" and managed
  // separately in VW.h.
  virtual void operator()(const InputType &input
                          , const Phrase &contextPhrase
                          , const AlignmentInfo &alignmentInfo
                          , Discriminative::Classifier &classifier
                          , Discriminative::FeatureVector &outFeatures) const = 0;

protected:
  std::vector<FactorType> m_sourceFactors, m_targetFactors;

  void UpdateRegister() {
    for(std::vector<std::string>::const_iterator it = m_usedBy.begin();
        it != m_usedBy.end(); it++) {
      s_features[*it].push_back(this);

      if(m_featureType == vwft_source) {
        s_sourceFeatures[*it].push_back(this);
      } else if (m_featureType == vwft_targetContext) {
        s_targetContextFeatures[*it].push_back(this);
        UpdateContextSize(*it);
      } else {
        s_targetFeatures[*it].push_back(this);
      }
    }
  }

private:
  void ParseUsedBy(const std::string &usedBy) {
    m_usedBy.clear();
    Tokenize(m_usedBy, usedBy, ",");
  }

  void UpdateContextSize(const std::string &usedBy);

  std::vector<std::string> m_usedBy;
  VWFeatureType m_featureType;
  static std::map<std::string, std::vector<VWFeatureBase*> > s_features;
  static std::map<std::string, std::vector<VWFeatureBase*> > s_sourceFeatures;
  static std::map<std::string, std::vector<VWFeatureBase*> > s_targetContextFeatures;
  static std::map<std::string, std::vector<VWFeatureBase*> > s_targetFeatures;

  static std::map<std::string, size_t> s_targetContextLength;
};

}

