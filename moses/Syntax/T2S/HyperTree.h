#pragma once

#include <map>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include "moses/Syntax/RuleTable.h"
#include "moses/Syntax/SymbolEqualityPred.h"
#include "moses/Syntax/SymbolHasher.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Terminal.h"
#include "moses/Util.h"
#include "moses/Word.h"

#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

class HyperTree: public RuleTable
{
public:
  class Node
  {
  public:
    typedef boost::unordered_map<std::vector<Factor*>, Node> Map;

    bool IsLeaf() const {
      return m_map.empty();
    }

    bool HasRules() const {
      return !m_targetPhraseCollection.IsEmpty();
    }

    void Prune(std::size_t tableLimit);
    void Sort(std::size_t tableLimit);

    Node *GetOrCreateChild(const HyperPath::NodeSeq &);

    const Node *GetChild(const HyperPath::NodeSeq &) const;

    const TargetPhraseCollection::shared_ptr GetTargetPhraseCollection() const
    return m_targetPhraseCollection;
  }

  TargetPhraseCollection::shared_ptr GetTargetPhraseCollection()
  return m_targetPhraseCollection;
}

const Map &GetMap() const
{
  return m_map;
}

private:
Map m_map;
TargetPhraseCollection m_targetPhraseCollection;
};

HyperTree(const RuleTableFF *ff) : RuleTable(ff) {}

const Node &GetRootNode() const
{
  return m_root;
}

private:
friend class RuleTrieCreator;

TargetPhraseCollection::shared_ptr GetOrCreateTargetPhraseCollection(
  const Word &sourceLHS, const Phrase &sourceRHS);

Node &GetOrCreateNode(const Phrase &sourceRHS);

void SortAndPrune(std::size_t);

Node m_root;
};

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
