#pragma once

#include <map>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

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
namespace S2T
{

class RuleTrieCYKPlus : public RuleTrie
{
public:
  class Node
  {
  public:
    typedef boost::unordered_map<Word, Node, SymbolHasher,
            SymbolEqualityPred> SymbolMap;

    bool IsLeaf() const {
      return m_sourceTermMap.empty() && m_nonTermMap.empty();
    }

    bool HasRules() const {
      return !m_targetPhraseCollection->IsEmpty();
    }

    void Prune(std::size_t tableLimit);
    void Sort(std::size_t tableLimit);

    Node *GetOrCreateChild(const Word &sourceTerm);
    Node *GetOrCreateNonTerminalChild(const Word &targetNonTerm);

    const Node *GetChild(const Word &sourceTerm) const;
    const Node *GetNonTerminalChild(const Word &targetNonTerm) const;

    TargetPhraseCollection::shared_ptr
    GetTargetPhraseCollection() const {
      return m_targetPhraseCollection;
    }

    TargetPhraseCollection::shared_ptr
    GetTargetPhraseCollection() {
      return m_targetPhraseCollection;
    }

    const SymbolMap &GetTerminalMap() const {
      return m_sourceTermMap;
    }

    const SymbolMap &GetNonTerminalMap() const {
      return m_nonTermMap;
    }

    Node() : m_targetPhraseCollection(new TargetPhraseCollection) {}

  private:
    SymbolMap m_sourceTermMap;
    SymbolMap m_nonTermMap;
    TargetPhraseCollection::shared_ptr m_targetPhraseCollection;
  };

  RuleTrieCYKPlus(const RuleTableFF *ff) : RuleTrie(ff) {}

  const Node &GetRootNode() const {
    return m_root;
  }

  bool HasPreterminalRule(const Word &) const;

private:
  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection
  (const Phrase &source, const TargetPhrase &target, const Word *sourceLHS);

  Node &GetOrCreateNode(const Phrase &source, const TargetPhrase &target,
                        const Word *sourceLHS);

  void SortAndPrune(std::size_t);

  Node m_root;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
