#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Word.h"
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
  bool m_binarized;
  Word m_send;
  Word m_send_nt;

public:
  TreeStructureFeature(const std::string &line)
    :StatefulFeatureFunction(0, line)
    , m_binarized(false) {
    ReadParameters();
    std::vector<FactorType> factors;
    factors.push_back(0);
    m_send.CreateFromString(Output, factors, "</s>", false);
    m_send_nt.CreateFromString(Output, factors, "SEND", true);
  }
  ~TreeStructureFeature() {
    delete m_constraints;
  };

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new TreeState(TreePointer());
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

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

  void Load(AllOptions::ptr const& opts);
};


}
