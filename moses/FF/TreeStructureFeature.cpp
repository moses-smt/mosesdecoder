#include "TreeStructureFeature.h"
#include "moses/StaticData.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/TargetPhrase.h"
#include <vector>
#include "moses/PP/TreeStructurePhraseProperty.h"

using namespace std;

namespace Moses
{

InternalTree::InternalTree(const std::string & line, const bool terminal):
    m_value_nt(0),
    m_isTerminal(terminal)
    {

    size_t found = line.find_first_of("[] ");

    if (found == line.npos) {
        m_value = line;
    }

    else {
        AddSubTree(line, 0);
    }
}

size_t InternalTree::AddSubTree(const std::string & line, size_t pos) {

    std::string value = "";
    char token = 0;

    while (token != ']' && pos != std::string::npos)
    {
        size_t oldpos = pos;
        pos = line.find_first_of("[] ", pos);
        if (pos == std::string::npos) break;
        token = line[pos];
        value = line.substr(oldpos,pos-oldpos);

        if (token == '[') {
            if (m_value.size() > 0) {
                TreePointer child(new InternalTree(value, false));
                m_children.push_back(child);
                pos = child->AddSubTree(line, pos+1);
            }
            else {
                if (value.size() > 0) {
                    m_value = value;
                }
                pos = AddSubTree(line, pos+1);
            }
        }
        else if (token == ' ' || token == ']') {
            if (value.size() > 0 && ! m_value.size() > 0) {
                m_value = value;
            }
            else if (value.size() > 0) {
                m_isTerminal = false;
                TreePointer child(new InternalTree(value, true));
                m_children.push_back(child);
            }
            if (token == ' ') {
                pos++;
            }
        }

        if (m_children.size() > 0) {
            m_isTerminal = false;
        }
    }

    if (pos == std::string::npos) {
        return line.size();
    }
    return min(line.size(),pos+1);

}

std::string InternalTree::GetString() const {

    std::string ret = " ";

    if (!m_isTerminal) {
        ret += "[";
    }

    ret += m_value;
    for (std::vector<TreePointer>::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
    {
        ret += (*it)->GetString();
    }

    if (!m_isTerminal) {
        ret += "]";
    }
    return ret;

}


void InternalTree::Combine(const std::vector<TreePointer> &previous) {

    std::vector<TreePointer>::iterator it;
    bool found = false;
    leafNT next_leafNT(this);
    for (std::vector<TreePointer>::const_iterator it_prev = previous.begin(); it_prev != previous.end(); ++it_prev) {
        found = next_leafNT(it);
        if (found) {
            *it = *it_prev;
        }
        else {
            std::cerr << "Warning: leaf nonterminal not found in rule; why did this happen?\n";
        }
    }
}


bool InternalTree::FlatSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetLabel() == label) {
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetLabel() == label) {
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(label, it2)) {
            it = it2;
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetLabel() == label) {
            parent = this;
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(label, it2, parent)) {
            it = it2;
            return true;
        }
    }
    return false;
}


bool InternalTree::FlatSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetNTLabel() == label) {
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetNTLabel() == label) {
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(label, it2)) {
            it = it2;
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->GetNTLabel() == label) {
            parent = this;
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(label, it2, parent)) {
            it = it2;
            return true;
        }
    }
    return false;
}


bool InternalTree::FlatSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(labels, it2)) {
            it = it2;
            return true;
        }
    }
    return false;
}

bool InternalTree::RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const {
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        if (std::binary_search(labels.begin(), labels.end(), (*it)->GetNTLabel())) {
            parent = this;
            return true;
        }
        std::vector<TreePointer>::const_iterator it2;
        if ((*it)->RecursiveSearch(labels, it2, parent)) {
            it = it2;
            return true;
        }
    }
    return false;
}


void TreeStructureFeature::Load() {

  // syntactic constraints can be hooked in here.
  m_constraints = NULL;
  m_labelset = NULL;

  StaticData &staticData = StaticData::InstanceNonConst();
  staticData.SetTreeStructure(this);
}


// define NT labels (ints) that are mapped from strings for quicker comparison.
void TreeStructureFeature::AddNTLabels(TreePointer root) const {
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
    TreePointer mytree (new InternalTree(*tree));

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

    std::vector<std::string> sparse_features;
    if (m_constraints) {
      sparse_features = m_constraints->SyntacticRules(mytree, previous_trees);
    }
    mytree->Combine(previous_trees);

    //sparse scores
    for (std::vector<std::string>::const_iterator feature=sparse_features.begin(); feature != sparse_features.end(); ++feature) {
      accumulator->PlusEquals(this, *feature, 1);
    }
    return new TreeState(mytree);
  }
  else {
    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
  }

}

}

