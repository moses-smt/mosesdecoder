#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include <boost/shared_ptr.hpp>
#include "util/generator.hh"
#include "util/exception.hh"

namespace Moses
{

class InternalTree;
typedef boost::shared_ptr<InternalTree> TreePointer;

class InternalTree
{
std::string m_value;
std::vector<TreePointer> m_children;
bool m_isTerminal;
public:
    InternalTree(const std::string & line, const bool terminal = false);
    size_t AddSubTree(const std::string & line, size_t start);

    std::string GetString() const;
    void Combine(const std::vector<TreePointer> &previous);
    const std::string & GetLabel() const {
        return m_value;
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


    // if found, 'it' is iterator to first tree node that matches search string
    bool FlatSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const;
    bool RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it) const;

    // if found, 'it' is iterator to first tree node that matches search string, and 'parent' to its parent node
    bool RecursiveSearch(const std::string & label, std::vector<TreePointer>::const_iterator & it, InternalTree const* &parent) const;

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

class SyntaxConstraintFeature : public StatefulFeatureFunction
{
public:
  SyntaxConstraintFeature(const std::string &line)
    :StatefulFeatureFunction(0, line) {}

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new TreeState(TreePointer());
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const {};
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const {};
  FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};
  FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

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

