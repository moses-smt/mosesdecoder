#pragma once

#include <map>
#include <vector>

#include <boost/unordered_map.hpp>

#include "moses/Syntax/RuleTable.h"
#include "moses/TargetPhraseCollection.h"

#include "HyperPath.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

// A HyperTree for representing a tree-to-string rule table.  See this paper:
//
//  Hui Zhang, Min Zhang, Haizhou Li, and Chew Lim Tan
//  "Fast Translation Rule Matching for Syntax-based Statistical Machine
//   Translation"
//  In proceedings of EMNLP 2009
//
class HyperTree : public RuleTable
{
public:
  class Node
  {
  public:
    typedef boost::unordered_map<HyperPath::NodeSeq, Node> Map;

    bool IsLeaf() const {
      return m_map.empty();
    }

    bool HasRules() const {
      return !m_targetPhraseCollection->IsEmpty();
    }

    void Prune(std::size_t tableLimit);
    void Sort(std::size_t tableLimit);

    Node *GetOrCreateChild(const HyperPath::NodeSeq &);

    const Node *GetChild(const HyperPath::NodeSeq &) const;

    TargetPhraseCollection::shared_ptr
    GetTargetPhraseCollection() const {
      return m_targetPhraseCollection;
    }

    TargetPhraseCollection::shared_ptr
    GetTargetPhraseCollection() {
      return m_targetPhraseCollection;
    }

    const Map &GetMap() const {
      return m_map;
    }

    Node() : m_targetPhraseCollection(new TargetPhraseCollection) { }

  private:
    Map m_map;
    TargetPhraseCollection::shared_ptr m_targetPhraseCollection;
  };

  HyperTree(const RuleTableFF *ff) : RuleTable(ff) { }

  const Node &GetRootNode() const {
    return m_root;
  }

private:
  friend class HyperTreeCreator;

  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(const HyperPath &);

  Node &GetOrCreateNode(const HyperPath &);

  void SortAndPrune(std::size_t);

  Node m_root;
};

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
