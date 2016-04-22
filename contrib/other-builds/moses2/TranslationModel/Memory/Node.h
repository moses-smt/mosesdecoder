/*
 * Node.h
 *
 *  Created on: 22 Apr 2016
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include "../../PhraseBased/TargetPhrases.h"
#include "../../System.h"

namespace Moses2
{
class System;

class Node
{
public:
  typedef boost::unordered_map<Word, Node, UnorderedComparer<Word>,
      UnorderedComparer<Word> > Children;

  Node();
  ~Node();
  void AddRule(Phrase &source, TargetPhrase *target);
  TargetPhrases *Find(const Phrase &source, size_t pos = 0) const;
  const Node *Find(const Word &word) const;

  const TargetPhrases *GetTargetPhrases() const
  { return m_targetPhrases; }

  void SortAndPrune(size_t tableLimit, MemPool &pool, System &system);

  const Children &GetChildren() const
  { return m_children; }

protected:
  Children m_children;
  TargetPhrases *m_targetPhrases;
  Phrase *m_source;
  std::vector<TargetPhrase*> *m_unsortedTPS;

  Node &AddRule(Phrase &source, TargetPhrase *target, size_t pos);

};

Node::Node() :
    m_targetPhrases(NULL), m_unsortedTPS(NULL)
{
}

Node::~Node()
{
}

void Node::AddRule(Phrase &source, TargetPhrase *target)
{
  AddRule(source, target, 0);
}

Node &Node::AddRule(Phrase &source,
    TargetPhrase *target, size_t pos)
{
  if (pos == source.GetSize()) {
    if (m_unsortedTPS == NULL) {
      m_unsortedTPS = new std::vector<TargetPhrase*>();
      m_source = &source;
    }

    m_unsortedTPS->push_back(target);
    return *this;
  }
  else {
    const Word &word = source[pos];
    Node &child = m_children[word];
    return child.AddRule(source, target, pos + 1);
  }
}

TargetPhrases *Node::Find(const Phrase &source,
    size_t pos) const
{
  assert(source.GetSize());
  if (pos == source.GetSize()) {
    return m_targetPhrases;
  }
  else {
    const Word &word = source[pos];
    //cerr << "word=" << word << endl;
    Children::const_iterator iter = m_children.find(word);
    if (iter == m_children.end()) {
      return NULL;
    }
    else {
      const Node &child = iter->second;
      return child.Find(source, pos + 1);
    }
  }
}

const Node *Node::Find(const Word &word) const
{
  Children::const_iterator iter = m_children.find(word);
  if (iter == m_children.end()) {
    return NULL;
  }
  else {
    const Node &child = iter->second;
    return &child;
  }

}

void Node::SortAndPrune(size_t tableLimit, MemPool &pool,
    System &system)
{
  BOOST_FOREACH(Children::value_type &val, m_children){
    Node &child = val.second;
    child.SortAndPrune(tableLimit, pool, system);
  }

  // prune target phrases in this node
  if (m_unsortedTPS) {
    m_targetPhrases = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, m_unsortedTPS->size());

    for (size_t i = 0; i < m_unsortedTPS->size(); ++i) {
      TargetPhrase *tp = (*m_unsortedTPS)[i];
      m_targetPhrases->AddTargetPhrase(*tp);
    }

    m_targetPhrases->SortAndPrune(tableLimit);
    system.featureFunctions.EvaluateAfterTablePruning(system.GetSystemPool(), *m_targetPhrases, *m_source);

    delete m_unsortedTPS;
  }
}

} // namespace

