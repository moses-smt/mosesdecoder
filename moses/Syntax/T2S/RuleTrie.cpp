#include "RuleTrie.h"

#include <map>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <boost/version.hpp>

#include "moses/NonTerminal.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/Util.h"
#include "moses/Word.h"

namespace Moses
{
namespace Syntax
{
namespace T2S
{

void RuleTrie::Node::Prune(std::size_t tableLimit)
{
  // Recusively prune child nodes.
  for (SymbolMap::iterator p = m_sourceTermMap.begin();
       p != m_sourceTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  for (SymbolMap::iterator p = m_nonTermMap.begin();
       p != m_nonTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }

  // Prune TargetPhraseCollections at this node.
  for (TPCMap::iterator p = m_targetPhraseCollections.begin();
       p != m_targetPhraseCollections.end(); ++p) {
    p->second->Prune(true, tableLimit);
  }
}

void RuleTrie::Node::Sort(std::size_t tableLimit)
{
  // Recusively sort child nodes.
  for (SymbolMap::iterator p = m_sourceTermMap.begin();
       p != m_sourceTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  for (SymbolMap::iterator p = m_nonTermMap.begin();
       p != m_nonTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }

  // Sort TargetPhraseCollections at this node.
  for (TPCMap::iterator p = m_targetPhraseCollections.begin();
       p != m_targetPhraseCollections.end(); ++p) {
    p->second->Sort(true, tableLimit);
  }
}

RuleTrie::Node*
RuleTrie::Node::
GetOrCreateChild(const Word &sourceTerm)
{
  return &m_sourceTermMap[sourceTerm];
}

RuleTrie::Node *
RuleTrie::
Node::
GetOrCreateNonTerminalChild(const Word &targetNonTerm)
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);

  return &m_nonTermMap[targetNonTerm];
}

TargetPhraseCollection::shared_ptr
RuleTrie::
Node::
GetOrCreateTargetPhraseCollection(const Word &sourceLHS)
{
  UTIL_THROW_IF2(!sourceLHS.IsNonTerminal(),
                 "Not a non-terminal: " << sourceLHS);
  TargetPhraseCollection::shared_ptr& foo
  = m_targetPhraseCollections[sourceLHS];
  if (!foo) foo.reset(new TargetPhraseCollection);
  return foo;
}

RuleTrie::Node const*
RuleTrie::
Node::
GetChild(const Word &sourceTerm) const
{
  UTIL_THROW_IF2(sourceTerm.IsNonTerminal(), "Not a terminal: " << sourceTerm);
  SymbolMap::const_iterator p = m_sourceTermMap.find(sourceTerm);
  return (p == m_sourceTermMap.end()) ? NULL : &p->second;
}

RuleTrie::Node const*
RuleTrie::
Node::
GetNonTerminalChild(const Word &targetNonTerm) const
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                 "Not a non-terminal: " << targetNonTerm);
  SymbolMap::const_iterator p = m_nonTermMap.find(targetNonTerm);
  return (p == m_nonTermMap.end()) ? NULL : &p->second;
}

TargetPhraseCollection::shared_ptr
RuleTrie::
GetOrCreateTargetPhraseCollection
( const Word &sourceLHS, const Phrase &sourceRHS )
{
  Node &currNode = GetOrCreateNode(sourceRHS);
  return currNode.GetOrCreateTargetPhraseCollection(sourceLHS);
}

RuleTrie::Node &
RuleTrie::
GetOrCreateNode(const Phrase &sourceRHS)
{
  const std::size_t size = sourceRHS.GetSize();

  Node *currNode = &m_root;
  for (std::size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = sourceRHS.GetWord(pos);

    if (word.IsNonTerminal()) {
      currNode = currNode->GetOrCreateNonTerminalChild(word);
    } else {
      currNode = currNode->GetOrCreateChild(word);
    }

    UTIL_THROW_IF2(currNode == NULL, "Node not found at position " << pos);
  }

  return *currNode;
}

void RuleTrie::SortAndPrune(std::size_t tableLimit)
{
  if (tableLimit) {
    m_root.Sort(tableLimit);
  }
}

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
