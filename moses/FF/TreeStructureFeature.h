#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "InternalTree.h"

namespace Moses
{

typedef int NTLabel;


// mapping from string nonterminal label to int representation.
// allows abstraction if multiple nonterminal strings should map to same label.
struct LabelSet {
public:
  std::map<std::string, NTLabel> string_to_label;
};


// class to implement language-specific syntactic constraints.
// the method SyntacticRules is given pointer to ScoreComponentCollection, so it can add sparse features itself.
class SyntaxConstraints
{
public:
  virtual void SyntacticRules(TreePointer root, const std::vector<TreePointer> &previous, const FeatureFunction* sp, ScoreComponentCollection* accumulator) = 0;
  virtual ~SyntaxConstraints() {};
};


class TreeStructureFeature : public StatefulFeatureFunction
{
  SyntaxConstraints* m_constraints;
  LabelSet* m_labelset;
public:
  TreeStructureFeature(const std::string &line)
    :StatefulFeatureFunction(0, line) {
    ReadParameters();
  }
  ~TreeStructureFeature() {
    delete m_constraints;
  };

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new TreeState(TreePointer());
  }

  void AddNTLabels(TreePointer root) const;

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const {};
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {};

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }


  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {
    UTIL_THROW(util::Exception, "Not implemented");
  };
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void Load();
};


}
