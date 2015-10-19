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

namespace Moses
{
namespace Syntax
{
namespace T2S
{

class RuleTrie: public RuleTable
{
public:
  class Node
  {
  public:
    typedef boost::unordered_map<Word, Node, SymbolHasher,
            SymbolEqualityPred> SymbolMap;

    typedef boost::unordered_map<Word, TargetPhraseCollection::shared_ptr,
            SymbolHasher, SymbolEqualityPred> TPCMap;

    bool IsLeaf() const {
      return m_sourceTermMap.empty() && m_nonTermMap.empty();
    }

    bool HasRules() const {
      return !m_targetPhraseCollections.empty();
    }

    void Prune(std::size_t tableLimit);
    void Sort(std::size_t tableLimit);

    Node *GetOrCreateChild(const Word &sourceTerm);
    Node *GetOrCreateNonTerminalChild(const Word &targetNonTerm);
    TargetPhraseCollection::shared_ptr GetOrCreateTargetPhraseCollection(const Word &);

    const Node *GetChild(const Word &sourceTerm) const;
    const Node *GetNonTerminalChild(const Word &targetNonTerm) const;

    TargetPhraseCollection::shared_ptr
    GetTargetPhraseCollection(const Word &sourceLHS) const {
      TPCMap::const_iterator p = m_targetPhraseCollections.find(sourceLHS);
      if (p != m_targetPhraseCollections.end())
        return p->second;
      else
        return TargetPhraseCollection::shared_ptr();
    }

    // FIXME IS there any reason to distinguish these two for T2S?
    const SymbolMap &GetTerminalMap() const {
      return m_sourceTermMap;
    }

    const SymbolMap &GetNonTerminalMap() const {
      return m_nonTermMap;
    }

  private:
    SymbolMap m_sourceTermMap;
    SymbolMap m_nonTermMap;
    TPCMap m_targetPhraseCollections;
  };

  RuleTrie(const RuleTableFF *ff) : RuleTable(ff) {}

  const Node &GetRootNode() const {
    return m_root;
  }

private:
  friend class RuleTrieCreator;

  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection
  (const Word &sourceLHS, const Phrase &sourceRHS);

  Node &GetOrCreateNode(const Phrase &sourceRHS);

  void SortAndPrune(std::size_t);

  Node m_root;
};

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
