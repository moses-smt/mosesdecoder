
#pragma once

#include <boost/unordered_map.hpp>
#include "Word.h"
#include "TargetPhrase.h"
#include "TargetPhrases.h"

class Phrase;

class Node
{
public:
  typedef boost::unordered_map<Word, Node, WordHasher> Children;

  Node();
  virtual ~Node();

  Node &GetOrCreate(const Phrase &source, size_t pos);
  const Node *Get(const Word &word) const;

  void AddTarget(TargetPhrase *target);
  const TargetPhrases &GetTargetPhrases() const {
    return m_tpColl;
  }

protected:
  Children m_children;
  TargetPhrases m_tpColl;
};


