#include "RuleTrieScope3.h"

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

void RuleTrieScope3::Node::Prune(std::size_t tableLimit)
{
  // Recusively prune child node values.
  for (TerminalMap::iterator p = m_terminalMap.begin();
       p != m_terminalMap.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  if (m_gapNode) {
    m_gapNode->Prune(tableLimit);
  }

  // Prune TargetPhraseCollections at this node.
  for (LabelMap::iterator p = m_labelMap.begin(); p != m_labelMap.end(); ++p) {
    p->second.Prune(true, tableLimit);
  }
}

void RuleTrieScope3::Node::Sort(std::size_t tableLimit)
{
  // Recusively sort child node values.
  for (TerminalMap::iterator p = m_terminalMap.begin();
       p != m_terminalMap.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  if (m_gapNode) {
    m_gapNode->Sort(tableLimit);
  }

  // Sort TargetPhraseCollections at this node.
  for (LabelMap::iterator p = m_labelMap.begin(); p != m_labelMap.end(); ++p) {
    p->second.Sort(true, tableLimit);
  }
}

RuleTrieScope3::Node *RuleTrieScope3::Node::GetOrCreateTerminalChild(
  const Word &sourceTerm)
{
  assert(!sourceTerm.IsNonTerminal());
  std::pair<TerminalMap::iterator, bool> result;
  result = m_terminalMap.insert(std::make_pair(sourceTerm, Node()));
  const TerminalMap::iterator &iter = result.first;
  Node &child = iter->second;
  return &child;
}

RuleTrieScope3::Node *RuleTrieScope3::Node::GetOrCreateNonTerminalChild(
  const Word &targetNonTerm)
{
  assert(targetNonTerm.IsNonTerminal());
  if (m_gapNode == NULL) {
    m_gapNode = new Node();
  }
  return m_gapNode;
}

TargetPhraseCollection &
RuleTrieScope3::Node::GetOrCreateTargetPhraseCollection(
  const TargetPhrase &target)
{
  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  const std::size_t rank = alignmentInfo.GetSize();

  std::vector<int> vec;
  vec.reserve(rank);

  m_labelTable.resize(rank);

  int i = 0;
  for (AlignmentInfo::const_iterator p = alignmentInfo.begin();
       p != alignmentInfo.end(); ++p) {
    std::size_t targetNonTermIndex = p->second;
    const Word &targetNonTerm = target.GetWord(targetNonTermIndex);
    vec.push_back(InsertLabel(i++, targetNonTerm));
  }

  return m_labelMap[vec];
}

TargetPhraseCollection &RuleTrieScope3::GetOrCreateTargetPhraseCollection(
  const Phrase &source, const TargetPhrase &target, const Word *sourceLHS)
{
  Node &currNode = GetOrCreateNode(source, target, sourceLHS);
  return currNode.GetOrCreateTargetPhraseCollection(target);
}

RuleTrieScope3::Node &RuleTrieScope3::GetOrCreateNode(
  const Phrase &source, const TargetPhrase &target, const Word */*sourceLHS*/)
{
  const std::size_t size = source.GetSize();

  const AlignmentInfo &alignmentInfo = target.GetAlignNonTerm();
  AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  Node *currNode = &m_root;
  for (std::size_t pos = 0 ; pos < size ; ++pos) {
    const Word &word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      assert(iterAlign != alignmentInfo.end());
      assert(iterAlign->first == pos);
      std::size_t targetNonTermInd = iterAlign->second;
      ++iterAlign;
      const Word &targetNonTerm = target.GetWord(targetNonTermInd);
      currNode = currNode->GetOrCreateNonTerminalChild(targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateTerminalChild(word);
    }

    assert(currNode != NULL);
  }

  return *currNode;
}

void RuleTrieScope3::SortAndPrune(std::size_t tableLimit)
{
  if (tableLimit) {
    m_root.Sort(tableLimit);
  }
}

bool RuleTrieScope3::HasPreterminalRule(const Word &w) const
{
  const Node::TerminalMap &map = m_root.GetTerminalMap();
  Node::TerminalMap::const_iterator p = map.find(w);
  return p != map.end() && p->second.HasRules();
}

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
