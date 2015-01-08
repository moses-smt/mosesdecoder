#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "FFState.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
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
    InternalTree(const std::string & line, size_t start, size_t len, const bool terminal);
    InternalTree(const std::string & line, const bool terminal = false);
    InternalTree(const InternalTree & tree):
        m_value(tree.m_value),
        m_isTerminal(tree.m_isTerminal) {
            const std::vector<TreePointer> & children = tree.m_children;
            for (std::vector<TreePointer>::const_iterator it = children.begin(); it != children.end(); it++) {
                m_children.push_back(boost::make_shared<InternalTree>(**it));
            }
        }
    size_t AddSubTree(const std::string & line, size_t start);

    std::string GetString(bool start = true) const;
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
            if ((*it).get()) { // normal pointer to same object that TreePointer points to
                $restart(tree = (*it).get());
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
            if ((*it).get()) {
                $restart(tree = (*it).get());
            }
        }
    }
    $stop;
};

// Python-like generator that yields the next nonterminal leaf on every call, and also stores the path from the root of the tree to the nonterminal
$generator(leafNTPath) {
    std::vector<TreePointer>::iterator it;
    InternalTree* tree;
    std::vector<InternalTree*> * path;
    leafNTPath(InternalTree* root = NULL, std::vector<InternalTree*> * orig = NULL): tree(root), path(orig) {}
    $emit(std::vector<TreePointer>::iterator)
    path->push_back(tree);
    for (it = tree->GetChildren().begin(); it !=tree->GetChildren().end(); ++it) {
        if (!(*it)->IsTerminal() && (*it)->GetLength() == 0) {
            path->push_back((*it).get());
            $yield(it);
            path->pop_back();
        }
        else if ((*it)->GetLength() > 0) {
            if ((*it).get()) {
                $restart(tree = (*it).get());
            }
        }
    }
    path->pop_back();
    $stop;
};


}