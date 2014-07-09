#pragma once

#include <string>
#include <map>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include <boost/shared_ptr.hpp>
#include "util/generator.hh"
#include "util/exception.hh"

namespace Moses
{

class InternalTree;
typedef boost::shared_ptr<InternalTree> TreePointer;
typedef int NTLabel;

class InternalTree
{
std::string m_value;
NTLabel m_value_nt;
std::vector<TreePointer> m_children;
bool m_isTerminal;
public:
    InternalTree(const std::string & line, const bool terminal = false);
    InternalTree(const InternalTree & tree):
        m_value(tree.m_value),
        m_isTerminal(tree.m_isTerminal) {
            const std::vector<TreePointer> & children = tree.m_children;
            for (std::vector<TreePointer>::const_iterator it = children.begin(); it != children.end(); it++) {
                TreePointer child (new InternalTree(**it));
                m_children.push_back(child);
            }
        }
    size_t AddSubTree(const std::string & line, size_t start);

    std::string GetString() const;
    void Combine(const std::vector<TreePointer> &previous);
    const std::string & GetLabel() const {
        return m_value;
    }

    // optionally identify label by int instead of string;
    // allows abstraction if multiple nonterminal strings should map to same label.
    const NTLabel & GetNTLabel() const {
        return m_value_nt;
    }

    void SetNTLabel(NTLabel value) {
        m_value_nt = value;
    }

    size_t GetLength() const {
        return m_children.size();
    }
    std::vector<TreePointer> & GetChildren() {
        return m_children;
    }
    void AddChild(TreePointer child) {
        m_children.push_back(child);
    }

    bool IsTerminal() const {
        return m_isTerminal;
    }

    bool IsLeafNT() const {
        return (!m_isTerminal && m_children.size() == 0);
    }

    // different methods to search a tree (either just direct children (FlatSearch) or all children (RecursiveSearch)) for constituents.
    // can be used for formulating syntax constraints.

    // if found, 'it' is iterator to first tree node that matches search string
    bool FlatSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const;
    bool RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const;

    // if found, 'it' is iterator to first tree node that matches search string, and 'parent' to its parent node
    bool RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const;

    // use NTLabel for search to reduce number of string comparisons / deal with synonymous labels
    // if found, 'it' is iterator to first tree node that matches search string
    bool FlatSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const;
    bool RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it) const;

    // if found, 'it' is iterator to first tree node that matches search string, and 'parent' to its parent node
    bool RecursiveSearch(const NTLabel & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const;

    // pass vector of possible labels to search
    // if found, 'it' is iterator to first tree node that matches search string
    bool FlatSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const;
    bool RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it) const;

    // if found, 'it' is iterator to first tree node that matches search string, and 'parent' to its parent node
    bool RecursiveSearch(const std::vector<NTLabel> & labels, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const;


};

// mapping from string nonterminal label to int representation.
// allows abstraction if multiple nonterminal strings should map to same label.
struct LabelSet
{
public:
    std::map<std::string, NTLabel> string_to_label;
};


// class to implement language-specific syntactic constraints.
// the method SyntacticRules must return a vector of strings (each identifying a constraint violation), which are then made into sparse features.
class SyntaxConstraints
{
public:
    virtual std::vector<std::string> SyntacticRules(TreePointer root, const std::vector<TreePointer> &previous) = 0;
    virtual ~SyntaxConstraints() {};
};


class TreeState : public FFState
{
  TreePointer m_tree;
public:
  TreeState(TreePointer tree)
    :m_tree(tree)
  {}

  TreePointer GetTree() const {
      return m_tree;
  }

  int Compare(const FFState& other) const {return 0;};
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
  ~TreeStructureFeature() {delete m_constraints;};

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
  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void Load();
};

// Python-like generator that yields next nonterminal leaf on every call
$generator(leafNT) {
    std::vector<TreePointer>::iterator it;
    InternalTree* tree;
    leafNT(InternalTree* root = 0): tree(root) {}
    $emit(std::vector<TreePointer>::iterator)
    for (it = tree->GetChildren().begin(); it !=tree->GetChildren().end(); ++it) {
        if (!(*it)->IsTerminal() && (*it)->GetLength() == 0) {
            $yield(it);
        }
        else if ((*it)->GetLength() > 0) {
            if (&(**it)) { // normal pointer to same object that TreePointer points to
                $restart(tree = &(**it));
            }
        }
    }
    $stop;
};


// Python-like generator that yields the parent of the next nonterminal leaf on every call
$generator(leafNTParent) {
    std::vector<TreePointer>::iterator it;
    InternalTree* tree;
    leafNTParent(InternalTree* root = 0): tree(root) {}
    $emit(InternalTree*)
    for (it = tree->GetChildren().begin(); it !=tree->GetChildren().end(); ++it) {
        if (!(*it)->IsTerminal() && (*it)->GetLength() == 0) {
            $yield(tree);
        }
        else if ((*it)->GetLength() > 0) {
            if (&(**it)) { // normal pointer to same object that TreePointer points to
                $restart(tree = &(**it));
            }
        }
    }
    $stop;
};


}

