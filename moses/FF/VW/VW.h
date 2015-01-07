#pragma once

#include <string>
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TranslationOptionList.h"
#include "moses/Util.h"
#include "Normalizer.h"
#include "Classifier.h"
#include "VWFeatureBase.h"

namespace Moses
{

const std::string VW_DUMMY_LABEL = "1111"; // VW does not use the actual label, other classifiers might

class VW : public StatelessFeatureFunction
{
public:
  VW(const std::string &line)
    :StatelessFeatureFunction(1, line), m_train(false)
  {
    ReadParameters();
    if (m_train) {
      m_trainer = new Discriminative::VWTrainer(m_modelPath);
    } else {
      m_predictorFactory = new Discriminative::VWPredictorFactory(m_modelPath, m_vwOptions);
    }

    if (! m_normalizer) {
      VERBOSE(1, "VW :: No loss function specified, normalizing using softmax.\n");
      m_normalizer = (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer();
    }
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}
  
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
                , const TranslationOptionList &translationOptionList) const
  {
    Discriminative::Classifier *classifier = m_train 
      ? m_trainer 
      : (Discriminative::Classifier *)m_predictorFactory->Acquire();
    
    UTIL_THROW_IF2(translationOptionList.size() == 0, "There are not translation options.");
    VERBOSE(2, "VW :: Evaluating translation options\n");
    
    const std::vector<VWFeatureBase*>& sourceFeatures = VWFeatureBase::GetSourceFeatures(GetScoreProducerDescription());
    
    const WordsRange &sourceRange = translationOptionList.Get(0)->GetSourceWordsRange();
    const InputPath  &inputPath   = translationOptionList.Get(0)->GetInputPath();
    
    for(size_t i = 0; i < sourceFeatures.size(); ++i) 
        (*sourceFeatures[i])(input, inputPath, sourceRange, classifier);

    const std::vector<VWFeatureBase*>& targetFeatures = VWFeatureBase::GetTargetFeatures(GetScoreProducerDescription());

    std::vector<float> losses(translationOptionList.size());

    std::vector<float>::iterator iterLoss;
    TranslationOptionList::const_iterator iterTransOpt;
    for(iterTransOpt = translationOptionList.begin(), iterLoss = losses.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt, ++iterLoss) {
     
      const TargetPhrase &targetPhrase = (*iterTransOpt)->GetTargetPhrase();
      for(size_t i = 0; i < targetFeatures.size(); ++i)
        (*targetFeatures[i])(input, inputPath, targetPhrase, classifier);

      *iterLoss = classifier->Predict(MakeTargetLabel(targetPhrase));
      // TODO handle training somehow
    }

    (*m_normalizer)(losses);

    for(iterTransOpt = translationOptionList.begin(), iterLoss = losses.begin() ;
        iterTransOpt != translationOptionList.end() ; ++iterTransOpt, ++iterLoss) {
      TranslationOption &transOpt = **iterTransOpt;
      
      std::vector<float> newScores(m_numScoreComponents);
      newScores[0] = *iterLoss;
    
      ScoreComponentCollection &scoreBreakDown = transOpt.GetScoreBreakdown();
      scoreBreakDown.PlusEquals(this, newScores);
      
      transOpt.UpdateScore();
    }
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}


  void SetParameter(const std::string& key, const std::string& value)
  {
    if (key == "train") {
      m_train = Scan<bool>(value);
    } else if (key == "path") {
      m_modelPath = value;
    } else if (key == "vw-options") {
      m_vwOptions = value;
    } else if (key == "loss") {
      m_normalizer = value == "logistic"
          ? (Discriminative::Normalizer *) new Discriminative::LogisticLossNormalizer()
          : (Discriminative::Normalizer *) new Discriminative::SquaredLossNormalizer();
    } else {
      StatelessFeatureFunction::SetParameter(key, value);
    }
  }

private:
  std::string MakeTargetLabel(const TargetPhrase &targetPhrase) const
  {
    return VW_DUMMY_LABEL;
  }

  bool m_train; // false means predict
  std::string m_modelPath;
  std::string m_vwOptions;
  Discriminative::Normalizer *m_normalizer = NULL;
  Discriminative::Classifier *m_trainer = NULL;
  Discriminative::VWPredictorFactory *m_predictorFactory = NULL;
};

}

