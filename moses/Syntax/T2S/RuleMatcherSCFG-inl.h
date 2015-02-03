#pragma once

namespace Moses
{
namespace Syntax
{
namespace T2S
{

template<typename Callback>
RuleMatcherSCFG<Callback>::RuleMatcherSCFG(const InputTree &inputTree,
                                           const RuleTrie &ruleTrie)
    : m_inputTree(inputTree)
    , m_ruleTrie(ruleTrie)
{
}

template<typename Callback>
void RuleMatcherSCFG<Callback>::EnumerateHyperedges(const InputTree::Node &node,
                                                    Callback &callback)
{
  const int start = static_cast<int>(node.pvertex.span.GetStartPos());
  m_hyperedge.head = const_cast<PVertex*>(&node.pvertex);
  m_hyperedge.tail.clear();
  Match(node, m_ruleTrie.GetRootNode(), start, callback);
}

template<typename Callback>
void RuleMatcherSCFG<Callback>::Match(const InputTree::Node &inNode,
                                      const RuleTrie::Node &trieNode,
                                      int start, Callback &callback)
{
  // Try to extend the current hyperedge tail by adding a tree node that is a
  // descendent of inNode and has a span starting at start.
  const std::vector<InputTree::Node*> &nodes = m_inputTree.nodesAtPos[start];
  for (std::vector<InputTree::Node*>::const_iterator p = nodes.begin();
       p != nodes.end(); ++p) {
    InputTree::Node &candidate = **p;
    // Is candidate a descendent of inNode?
    if (!IsDescendent(candidate, inNode)) {
      continue;
    }
    // Get the appropriate SymbolMap (according to whether candidate is a
    // terminal or non-terminal map) from the current rule trie node.
    const RuleTrie::Node::SymbolMap *map = NULL;
    if (candidate.children.empty()) {
      map = &(trieNode.GetTerminalMap());
    } else {
      map = &(trieNode.GetNonTerminalMap());
    }
    // Test if the current rule prefix can be extended by candidate's symbol.
    RuleTrie::Node::SymbolMap::const_iterator q =
        map->find(candidate.pvertex.symbol);
    if (q == map->end()) {
      continue;
    }
    const RuleTrie::Node &newTrieNode = q->second;
    // Add the candidate node to the tail.
    m_hyperedge.tail.push_back(&candidate.pvertex);
    // Have we now covered the full span of inNode?
    if (candidate.pvertex.span.GetEndPos() == inNode.pvertex.span.GetEndPos()) {
      // Check if the trie node has any rules with a LHS that match inNode.
      const Word &lhs = inNode.pvertex.symbol;
      const TargetPhraseCollection *tpc =
          newTrieNode.GetTargetPhraseCollection(lhs);
      if (tpc) {
        m_hyperedge.label.translations = tpc;
        callback(m_hyperedge);
      }
    } else {
      // Recursive step.
      int newStart = candidate.pvertex.span.GetEndPos()+1;
      Match(inNode, newTrieNode, newStart, callback);
    }
    m_hyperedge.tail.pop_back();
  }
}

// Return true iff x is a descendent of y; false otherwise.
template<typename Callback>
bool RuleMatcherSCFG<Callback>::IsDescendent(const InputTree::Node &x,
                                             const InputTree::Node &y)
{
  const std::size_t xStart = x.pvertex.span.GetStartPos();
  const std::size_t yStart = y.pvertex.span.GetStartPos();
  const std::size_t xEnd = x.pvertex.span.GetEndPos();
  const std::size_t yEnd = y.pvertex.span.GetEndPos();
  if (xStart < yStart || xEnd > yEnd) {
    return false;
  }
  if (xStart > yStart || xEnd < yEnd) {
    return true;
  }
  // x and y both cover the same span.
  const InputTree::Node *z = &y;
  while (z->children.size() == 1) {
    z = z->children[0];
    if (z == &x) {
      return true;
    }
  }
  return false;
}

}  // namespace T2S
}  // namespace Syntax
}  // namespace Moses
