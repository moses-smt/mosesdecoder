#include "TreeStructureFeature.h"
#include "moses/StaticData.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/ChartHypothesis.h"
#include <vector>
#include "moses/PP/TreeStructurePhraseProperty.h"

namespace Moses
{

void TreeStructureFeature::Load()
{

  // syntactic constraints can be hooked in here.
  m_constraints = NULL;
  m_labelset = NULL;

  StaticData &staticData = StaticData::InstanceNonConst();
  staticData.SetTreeStructure(this);
}


// define NT labels (ints) that are mapped from strings for quicker comparison.
void TreeStructureFeature::AddNTLabels(TreePointer root) const
{
  std::string label = root->GetLabel();

  if (root->IsTerminal()) {
    return;
  }

  std::map<std::string, NTLabel>::const_iterator it = m_labelset->string_to_label.find(label);
  if (it != m_labelset->string_to_label.end()) {
    root->SetNTLabel(it->second);
  }

  std::vector<TreePointer> children = root->GetChildren();
  for (std::vector<TreePointer>::const_iterator it2 = children.begin(); it2 != children.end(); ++it2) {
    AddNTLabels(*it2);
  }
}

FFState* TreeStructureFeature::EvaluateWhenApplied(const ChartHypothesis& cur_hypo
    , int featureID /* used to index the state in the previous hypotheses */
    , ScoreComponentCollection* accumulator) const
{
  if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {
    const std::string *tree = property->GetValueString();
    TreePointer mytree (boost::make_shared<InternalTree>(*tree));

    if (m_labelset) {
      AddNTLabels(mytree);
    }

    //get subtrees (in target order)
    std::vector<TreePointer> previous_trees;
    for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
      const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
      if (word.IsNonTerminal()) {
        size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
        const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
        const TreeState* prev = dynamic_cast<const TreeState*>(prevHypo->GetFFState(featureID));
        const TreePointer prev_tree = prev->GetTree();
        previous_trees.push_back(prev_tree);
      }
    }

    if (m_constraints) {
      m_constraints->SyntacticRules(mytree, previous_trees, this, accumulator);
    }
    mytree->Combine(previous_trees);

    return new TreeState(mytree);
  } else {
    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
  }

}

}
