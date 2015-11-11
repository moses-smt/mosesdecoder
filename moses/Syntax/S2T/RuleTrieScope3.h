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
#include "moses/Util.h"
#include "moses/Word.h"

#include "RuleTrie.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class RuleTrieScope3 : public RuleTrie
{
public:
  class Node
  {
  public:
    typedef std::vector<std::vector<Word> > LabelTable;

    typedef boost::unordered_map<Word, Node, SymbolHasher,
            SymbolEqualityPred> TerminalMap;

    typedef boost::unordered_map<std::vector<int>,
            TargetPhraseCollection::shared_ptr> LabelMap;

    ~Node() {
      delete m_gapNode;
    }

    const LabelTable &GetLabelTable() const {
      return m_labelTable;
    }

    const LabelMap &GetLabelMap() const {
      return m_labelMap;
    }

    const TerminalMap &GetTerminalMap() const {
      return m_terminalMap;
    }

    const Node *GetNonTerminalChild() const {
      return m_gapNode;
    }

    Node *GetOrCreateTerminalChild(const Word &sourceTerm);

    Node *GetOrCreateNonTerminalChild(const Word &targetNonTerm);

    TargetPhraseCollection::shared_ptr
    GetOrCreateTargetPhraseCollection(const TargetPhrase &);

    bool IsLeaf() const {
      return m_terminalMap.empty() && m_gapNode == NULL;
    }

    bool HasRules() const {
      return !m_labelMap.empty();
    }

    void Prune(std::size_t tableLimit);
    void Sort(std::size_t tableLimit);

  private:
    friend class RuleTrieScope3;

    Node() : m_gapNode(NULL) {}

    int InsertLabel(int i, const Word &w) {
      std::vector<Word> &inner = m_labelTable[i];
      for (std::size_t j = 0; j < inner.size(); ++j) {
        if (inner[j] == w) {
          return j;
        }
      }
      inner.push_back(w);
      return inner.size()-1;
    }

    LabelTable m_labelTable;
    LabelMap m_labelMap;
    TerminalMap m_terminalMap;
    Node *m_gapNode;
  };

  RuleTrieScope3(const RuleTableFF *ff) : RuleTrie(ff) {}

  const Node &GetRootNode() const {
    return m_root;
  }

  bool HasPreterminalRule(const Word &) const;

private:
  TargetPhraseCollection::shared_ptr
  GetOrCreateTargetPhraseCollection(const Phrase &source,
                                    const TargetPhrase &target,
                                    const Word *sourceLHS);

  Node &GetOrCreateNode(const Phrase &source, const TargetPhrase &target,
                        const Word *sourceLHS);

  void SortAndPrune(std::size_t);

  Node m_root;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
