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
#include "../../Phrase.h"

namespace Moses2
{
class System;

namespace PtMem
{

template<class WORD, class SP, class TP, class TPS>
class Node
{
public:
  typedef boost::unordered_map<size_t, Node> Children;

  Node()
    :m_targetPhrases(NULL)
    ,m_unsortedTPS(NULL)
  {}

  ~Node()
  {}

  void AddRule(const std::vector<FactorType> &factors, SP &source, TP *target) {
    AddRule(factors, source, target, 0);
  }

  TPS *Find(const std::vector<FactorType> &factors, const SP &source, size_t pos = 0) const {
    assert(source.GetSize());
    if (pos == source.GetSize()) {
      return m_targetPhrases;
    } else {
      const WORD &word = source[pos];
      //cerr << "word=" << word << endl;
      typename Children::const_iterator iter = m_children.find(word.hash(factors));
      if (iter == m_children.end()) {
        return NULL;
      } else {
        const Node &child = iter->second;
        return child.Find(factors, source, pos + 1);
      }
    }
  }

  const Node *Find(const std::vector<FactorType> &factors, const WORD &word) const {
    typename Children::const_iterator iter = m_children.find(word.hash(factors));
    if (iter == m_children.end()) {
      return NULL;
    } else {
      const Node &child = iter->second;
      return &child;
    }
  }

  const TPS *GetTargetPhrases() const {
    return m_targetPhrases;
  }

  void SortAndPrune(size_t tableLimit, MemPool &pool, System &system) {
    BOOST_FOREACH(typename Children::value_type &val, m_children) {
      Node &child = val.second;
      child.SortAndPrune(tableLimit, pool, system);
    }

    // prune target phrases in this node
    if (m_unsortedTPS) {
      m_targetPhrases = new (pool.Allocate<TPS>()) TPS(pool, m_unsortedTPS->size());

      for (size_t i = 0; i < m_unsortedTPS->size(); ++i) {
        TP *tp = (*m_unsortedTPS)[i];
        m_targetPhrases->AddTargetPhrase(*tp);
      }

      m_targetPhrases->SortAndPrune(tableLimit);
      system.featureFunctions.EvaluateAfterTablePruning(system.GetSystemPool(), *m_targetPhrases, *m_source);

      delete m_unsortedTPS;
    }
  }

  const Children &GetChildren() const {
    return m_children;
  }

  void Debug(std::ostream &out, const System &system) const {
    BOOST_FOREACH(const typename Children::value_type &valPair, m_children) {
      const WORD &word = valPair.first;
      //std::cerr << word << "(" << word.hash() << ") ";
    }
  }
protected:
  Children m_children;
  TPS *m_targetPhrases;
  Phrase<WORD> *m_source;
  std::vector<TP*> *m_unsortedTPS;

  Node &AddRule(const std::vector<FactorType> &factors, SP &source, TP *target, size_t pos) {
    if (pos == source.GetSize()) {
      if (m_unsortedTPS == NULL) {
        m_unsortedTPS = new std::vector<TP*>();
        m_source = &source;
      }

      m_unsortedTPS->push_back(target);
      return *this;
    } else {
      const WORD &word = source[pos];
      Node &child = m_children[word.hash(factors)];
      //std::cerr << "added " << word << " " << &child << " from " << this << std::endl;

      return child.AddRule(factors, source, target, pos + 1);
    }
  }

};


}
} // namespace

