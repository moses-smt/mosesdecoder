#pragma once

#include <vector>

#include "moses/Syntax/S2T/RuleTrieScope3.h"
#include "moses/Util.h"

#include "SentenceMap.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

struct PatternApplicationTrie;

typedef std::vector<const PatternApplicationTrie*> PatternApplicationKey;

struct PatternApplicationTrie {
public:
  PatternApplicationTrie(int start, int end, const RuleTrieScope3::Node &node,
                         const PVertex *pvertex, PatternApplicationTrie *parent)
    : m_start(start)
    , m_end(end)
    , m_node(&node)
    , m_pvertex(pvertex)
    , m_parent(parent)
    , m_highestTerminalNode(0)
    , m_lowestTerminalNode(0) {}

  ~PatternApplicationTrie() {
    RemoveAllInColl(m_children);
  }

  int Depth() const;

  bool IsGapNode() const {
    return m_end == -1;
  }
  bool IsTerminalNode() const {
    return m_end != -1;
  }

  const PatternApplicationTrie *GetHighestTerminalNode() const;
  const PatternApplicationTrie *GetLowestTerminalNode() const;

  void DetermineStartRange(int, int &, int &) const;
  void DetermineEndRange(int, int &, int &) const;

  void Extend(const RuleTrieScope3::Node &node, int minPos,
              const SentenceMap &sentMap, bool followsGap);

  void ReadOffPatternApplicationKey(PatternApplicationKey &) const;

  int m_start;
  int m_end;
  const RuleTrieScope3::Node *m_node;
  const PVertex *m_pvertex;
  PatternApplicationTrie *m_parent;
  std::vector<PatternApplicationTrie*> m_children;
  mutable const PatternApplicationTrie *m_highestTerminalNode;
  mutable const PatternApplicationTrie *m_lowestTerminalNode;
};

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
