#pragma once

#include <string>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "StatelessFeatureFunction.h"
#include "FFState.h"
#include "moses/Factor.h"

namespace Moses
{


class SoftSourceSyntacticConstraintsFeature : public StatelessFeatureFunction
{

public:

  SoftSourceSyntacticConstraintsFeature(const std::string &line);

  ~SoftSourceSyntacticConstraintsFeature() {
    for (boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::iterator iter=m_labelPairProbabilities.begin();
         iter!=m_labelPairProbabilities.end(); ++iter) {
      delete iter->second;
    }
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);
  
  void Load();

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {
    targetPhrase.SetRuleSource(source);
  };

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const
  {}

  void EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {};

  void EvaluateWhenApplied(
    const ChartHypothesis& cur_hypo,
    ScoreComponentCollection* accumulator) const
  {};


protected:

  std::string m_sourceLabelSetFile;
  std::string m_coreSourceLabelSetFile;
  std::string m_targetSourceLHSJointCountFile;
  std::string m_unknownLeftHandSideFile;
  bool m_useCoreSourceLabels;
  bool m_useLogprobs;
  bool m_useSparse;
  bool m_noMismatches;

  boost::unordered_map<std::string,size_t> m_sourceLabels;
  std::vector<std::string> m_sourceLabelsByIndex;
  std::vector<std::string> m_sourceLabelsByIndex_RHS_1;
  std::vector<std::string> m_sourceLabelsByIndex_RHS_0;
  std::vector<std::string> m_sourceLabelsByIndex_LHS_1;
  std::vector<std::string> m_sourceLabelsByIndex_LHS_0;
  boost::unordered_set<size_t> m_coreSourceLabels;
  boost::unordered_map<const Factor*,size_t> m_sourceLabelIndexesByFactor;
  size_t m_GlueTopLabel;
//  mutable size_t m_XRHSLabel;
//  mutable size_t m_XLHSLabel;

  boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* > m_labelPairProbabilities;
  boost::unordered_map<size_t,float> m_unknownLHSProbabilities;
  float m_smoothingWeight;
  float m_unseenLHSSmoothingFactorForUnknowns;

  void LoadSourceLabelSet();
  void LoadCoreSourceLabelSet();
  void LoadTargetSourceLeftHandSideJointCountFile();

  void LoadLabelSet(std::string &filename, boost::unordered_set<size_t> &labelSet);

  std::pair<float,float> GetLabelPairProbabilities(const Factor* target,
      const size_t source) const;

};


}

