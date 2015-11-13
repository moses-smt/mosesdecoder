#include "PatternApplicationTrie.h"

#include "moses/Syntax/PVertex.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

int PatternApplicationTrie::Depth() const
{
  if (m_parent) {
    return m_parent->Depth() + 1;
  }
  return 0;
}

const PatternApplicationTrie *
PatternApplicationTrie::GetHighestTerminalNode() const
{
  // Check if result has been cached.
  if (m_highestTerminalNode) {
    return m_highestTerminalNode;
  }
  // It doesn't really make sense to call this on the root node.  Just return 0.
  if (!m_parent) {
    return 0;
  }
  // Is this the highest non-root node?
  if (!m_parent->m_parent) {
    if (IsTerminalNode()) {
      m_highestTerminalNode = this;
      return this;
    } else {
      return 0;
    }
  }
  // This is not the highest non-root node, so ask parent node.
  if (const PatternApplicationTrie *p = m_parent->GetHighestTerminalNode()) {
    m_highestTerminalNode = p;
    return p;
  }
  // There are no terminal nodes higher than this node.
  if (IsTerminalNode()) {
    m_highestTerminalNode = this;
  }
  return m_highestTerminalNode;
}

const PatternApplicationTrie *
PatternApplicationTrie::GetLowestTerminalNode() const
{
  // Check if result has been cached.
  if (m_lowestTerminalNode) {
    return m_lowestTerminalNode;
  }
  // It doesn't really make sense to call this on the root node.  Just return 0.
  if (!m_parent) {
    return 0;
  }
  // Is this a terminal node?
  if (IsTerminalNode()) {
    m_lowestTerminalNode = this;
    return this;
  }
  // Is this the highest non-root node?
  if (!m_parent->m_parent) {
    return 0;
  }
  // Ask parent node.
  return m_parent->GetLowestTerminalNode();
}

// A node corresponds to a rule pattern that has been partially applied to a
// sentence (the terminals have fixed positions, but the spans of gap symbols
// may be unknown).  This function determines the range of possible start
// values for the partially-applied pattern.
void PatternApplicationTrie::DetermineStartRange(int sentenceLength,
    int &minStart,
    int &maxStart) const
{
  // Find the leftmost terminal symbol, if any.
  const PatternApplicationTrie *n = GetHighestTerminalNode();
  if (!n) {
    // The pattern contains only gap symbols.
    minStart = 0;
    maxStart = sentenceLength-Depth();
    return;
  }
  assert(n->m_parent);
  if (!n->m_parent->m_parent) {
    // The pattern begins with a terminal symbol so the start position is
    // fixed.
    minStart = n->m_start;
    maxStart = n->m_start;
  } else {
    // The pattern begins with a gap symbol but it contains at least one
    // terminal symbol.  The maximum start position is the start position of
    // the leftmost terminal minus one position for each leading gap symbol.
    minStart = 0;
    maxStart = n->m_start - (n->Depth()-1);
  }
}

// A node corresponds to a rule pattern that has been partially applied to a
// sentence (the terminals have fixed positions, but the spans of gap symbols
// may be unknown).  This function determines the range of possible end values
// for the partially-applied pattern.
void PatternApplicationTrie::DetermineEndRange(int sentenceLength,
    int &minEnd,
    int &maxEnd) const
{
  // Find the rightmost terminal symbol, if any.
  const PatternApplicationTrie *n = GetLowestTerminalNode();
  if (!n) {
    // The pattern contains only gap symbols.
    minEnd = Depth()-1;
    maxEnd = sentenceLength-1;
    return;
  }
  if (n == this) {
    // The pattern ends with a terminal symbol so the end position is fixed.
    minEnd = m_end;
    maxEnd = m_end;
  } else {
    // The pattern ends with a gap symbol but it contains at least one terminal
    // symbol.  The minimum end position is the end position of the rightmost
    // terminal + one position for each trailing gap symbol.
    minEnd = n->m_end + (Depth()-n->Depth());
    maxEnd = sentenceLength-1;
  }
}

void PatternApplicationTrie::Extend(const RuleTrieScope3::Node &node,
                                    int minPos, const SentenceMap &sentMap,
                                    bool followsGap)
{
  const RuleTrieScope3::Node::TerminalMap &termMap = node.GetTerminalMap();
  for (RuleTrieScope3::Node::TerminalMap::const_iterator p = termMap.begin();
       p != termMap.end(); ++p) {
    const Word &word = p->first;
    const RuleTrieScope3::Node &child = p->second;
    SentenceMap::const_iterator q = sentMap.find(word);
    if (q == sentMap.end()) {
      continue;
    }
    for (std::vector<const PVertex *>::const_iterator r = q->second.begin();
         r != q->second.end(); ++r) {
      const PVertex *v = *r;
      std::size_t start = v->span.GetStartPos();
      std::size_t end = v->span.GetEndPos();
      if (start == (std::size_t)minPos ||
          (followsGap && start > (std::size_t)minPos) ||
          minPos == -1) {
        PatternApplicationTrie *subTrie =
          new PatternApplicationTrie(start, end, child, v, this);
        subTrie->Extend(child, end+1, sentMap, false);
        m_children.push_back(subTrie);
      }
    }
  }

  const RuleTrieScope3::Node *child = node.GetNonTerminalChild();
  if (!child) {
    return;
  }
  int start = followsGap ? -1 : minPos;
  PatternApplicationTrie *subTrie =
    new PatternApplicationTrie(start, -1, *child, 0, this);
  int newMinPos = (minPos == -1 ? 1 : minPos+1);
  subTrie->Extend(*child, newMinPos, sentMap, true);
  m_children.push_back(subTrie);
}

void PatternApplicationTrie::ReadOffPatternApplicationKey(
  PatternApplicationKey &key) const
{
  const int depth = Depth();
  key.resize(depth);
  const PatternApplicationTrie *p = this;
  std::size_t i = depth-1;
  while (p->m_parent != 0) {
    key[i--] = p;
    p = p->m_parent;
  }
}

}  // namespace S2T
}  // namespace Moses
}  // namespace Syntax
