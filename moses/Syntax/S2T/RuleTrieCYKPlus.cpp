#include "RuleTrieCYKPlus.h"

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
namespace S2T
{

void RuleTrieCYKPlus::Node::Prune(std::size_t tableLimit)
{
  // recusively prune
  for (SymbolMap::iterator p = m_sourceTermMap.begin();
       p != m_sourceTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  for (SymbolMap::iterator p = m_nonTermMap.begin();
       p != m_nonTermMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  m_targetPhraseCollection.Prune(true, tableLimit);
}

void RuleTrieCYKPlus::Node::Sort(std::size_t tableLimit)
{
  // recusively sort
  for (SymbolMap::iterator p = m_sourceTermMap.begin();
       p != m_sourceTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  for (SymbolMap::iterator p = m_nonTermMap.begin();
       p != m_nonTermMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }

  // prune TargetPhraseCollection in this node
  m_targetPhraseCollection.Sort(true, tableLimit);
}

RuleTrieCYKPlus::Node *RuleTrieCYKPlus::Node::GetOrCreateChild(
    const Word &sourceTerm)
{
  return &m_sourceTermMap[sourceTerm];
}

RuleTrieCYKPlus::Node *RuleTrieCYKPlus::Node::GetOrCreateNonTerminalChild(const Word &targetNonTerm)
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                  "Not a non-terminal: " << targetNonTerm);

  return &m_nonTermMap[targetNonTerm];
}

const RuleTrieCYKPlus::Node *RuleTrieCYKPlus::Node::GetChild(
    const Word &sourceTerm) const
{
  UTIL_THROW_IF2(sourceTerm.IsNonTerminal(),
		  "Not a terminal: " << sourceTerm);

  SymbolMap::const_iterator p = m_sourceTermMap.find(sourceTerm);
  return (p == m_sourceTermMap.end()) ? NULL : &p->second;
}

const RuleTrieCYKPlus::Node *RuleTrieCYKPlus::Node::GetNonTerminalChild(
    const Word &targetNonTerm) const
{
  UTIL_THROW_IF2(!targetNonTerm.IsNonTerminal(),
                  "Not a non-terminal: " << targetNonTerm);

  SymbolMap::const_iterator p = m_nonTermMap.find(targetNonTerm);
  return (p == m_nonTermMap.end()) ? NULL : &p->second;
}

TargetPhraseCollection &RuleTrieCYKPlus::GetOrCreateTargetPhraseCollection(
    const Phrase &source, const TargetPhrase &target, const Word *sourceLHS)
{
  Node &currNode = GetOrCreateNode(source, target, sourceLHS);
  return currNode.GetTargetPhraseCollection();
}

RuleTrieCYKPlus::Node &RuleTrieCYKPlus::GetOrCreateNode(
    const Phrase &source, const TargetPhrase &target, const Word *sourceLHS)
{
  const std::size_t size = source.GetSize();

  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  Node *currNode = &m_root;
  for (std::size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      UTIL_THROW_IF2(iterAlign == alignmentInfo.end(),
    		  "No alignment for non-term at position " << pos);
      UTIL_THROW_IF2(iterAlign->first != pos,
    		  "Alignment info incorrect at position " << pos);
      std::size_t targetNonTermInd = iterAlign->second;
      ++iterAlign;
      const Word &targetNonTerm = target.GetWord(targetNonTermInd);
      currNode = currNode->GetOrCreateNonTerminalChild(targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateChild(word);
    }

    UTIL_THROW_IF2(currNode == NULL, "Node not found at position " << pos);
  }

  return *currNode;
}

void RuleTrieCYKPlus::SortAndPrune(std::size_t tableLimit)
{
  if (tableLimit) {
    m_root.Sort(tableLimit);
  }
}

bool RuleTrieCYKPlus::HasPreterminalRule(const Word &w) const
{
  const Node::SymbolMap &map = m_root.GetTerminalMap();
  Node::SymbolMap::const_iterator p = map.find(w);
  return p != map.end() && p->second.HasRules();
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
