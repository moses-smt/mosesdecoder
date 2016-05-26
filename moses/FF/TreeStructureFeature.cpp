#include "TreeStructureFeature.h"
#include "moses/StaticData.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/ChartHypothesis.h"
#include <vector>
#include "moses/PP/TreeStructurePhraseProperty.h"

namespace Moses
{

void TreeStructureFeature::Load(AllOptions::ptr const& opts)
{
  m_options = opts;

  // syntactic constraints can be hooked in here.
  m_constraints = NULL;

  StaticData &staticData = StaticData::InstanceNonConst();
  staticData.SetTreeStructure(this);
}


FFState* TreeStructureFeature::EvaluateWhenApplied(const ChartHypothesis& cur_hypo
    , int featureID /* used to index the state in the previous hypotheses */
    , ScoreComponentCollection* accumulator) const
{
  if (const PhraseProperty *property = cur_hypo.GetCurrTargetPhrase().GetProperty("Tree")) {
    const std::string *tree = property->GetValueString();
    TreePointer mytree (boost::make_shared<InternalTree>(*tree));

    //get subtrees (in target order)
    std::vector<TreePointer> previous_trees;
    for (size_t pos = 0; pos < cur_hypo.GetCurrTargetPhrase().GetSize(); ++pos) {
      const Word &word = cur_hypo.GetCurrTargetPhrase().GetWord(pos);
      if (word.IsNonTerminal()) {
        size_t nonTermInd = cur_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap()[pos];
        const ChartHypothesis *prevHypo = cur_hypo.GetPrevHypo(nonTermInd);
        const TreeState* prev = static_cast<const TreeState*>(prevHypo->GetFFState(featureID));
        const TreePointer prev_tree = prev->GetTree();
        previous_trees.push_back(prev_tree);
      }
    }

    if (m_constraints) {
      m_constraints->SyntacticRules(mytree, previous_trees, this, accumulator);
    }
    mytree->Combine(previous_trees);

    bool full_sentence = (mytree->GetChildren().back()->GetLabel() == m_send || (mytree->GetChildren().back()->GetLabel() == m_send_nt && mytree->GetChildren().back()->GetChildren().back()->GetLabel() == m_send));
    if (m_binarized && full_sentence) {
      mytree->Unbinarize();
    }

    return new TreeState(mytree);
  } else {
    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
  }

}

void TreeStructureFeature::SetParameter(const std::string& key, const std::string& value)
{
  std::cerr << "setting: " << this->GetScoreProducerDescription() << " - " << key << "\n";
  if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else if (key == "filterable") { //ignore
  } else if (key == "binarized") { // if trees have been binarized before learning translation model; output unbinarized trees
    m_binarized = true;
  } else {
    UTIL_THROW(util::Exception, "Unknown argument " << key << "=" << value);
  }
}

}
