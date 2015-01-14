#include "StsgRule.h"

#include "Node.h"
#include "Subgraph.h"
#include "SyntaxTree.h"

#include <algorithm>

namespace Moses
{
namespace GHKM
{

StsgRule::StsgRule(const Subgraph &fragment)
  : m_targetSide(fragment, true)
{
  // Source side

  const std::set<const Node *> &sinkNodes = fragment.GetLeaves();

  // Collect the subset of sink nodes that excludes target nodes with
  // empty spans.
  std::vector<const Node *> productiveSinks;
  productiveSinks.reserve(sinkNodes.size());
  for (std::set<const Node *>::const_iterator p = sinkNodes.begin();
       p != sinkNodes.end(); ++p) {
    const Node *sink = *p;
    if (!sink->GetSpan().empty()) {
      productiveSinks.push_back(sink);
    }
  }

  // Sort them into the order defined by their spans.
  std::sort(productiveSinks.begin(), productiveSinks.end(), PartitionOrderComp);

  // Build a map from target nodes to source-order indices, so that we
  // can construct the Alignment object later.
  std::map<const Node *, std::vector<int> > sinkToSourceIndices;
  std::map<const Node *, int> nonTermSinkToSourceIndex;

  m_sourceSide.reserve(productiveSinks.size());
  int srcIndex = 0;
  int nonTermCount = 0;
  for (std::vector<const Node *>::const_iterator p = productiveSinks.begin();
       p != productiveSinks.end(); ++p, ++srcIndex) {
    const Node &sink = **p;
    if (sink.GetType() == TREE) {
      m_sourceSide.push_back(Symbol("X", NonTerminal));
      sinkToSourceIndices[&sink].push_back(srcIndex);
      nonTermSinkToSourceIndex[&sink] = nonTermCount++;
    } else {
      assert(sink.GetType() == SOURCE);
      m_sourceSide.push_back(Symbol(sink.GetLabel(), Terminal));
      // Add all aligned target words to the sinkToSourceIndices map
      const std::vector<Node *> &parents(sink.GetParents());
      for (std::vector<Node *>::const_iterator q = parents.begin();
           q != parents.end(); ++q) {
        if ((*q)->GetType() == TARGET) {
          sinkToSourceIndices[*q].push_back(srcIndex);
        }
      }
    }
  }

  // Alignment

  std::vector<const Node *> targetLeaves;
  m_targetSide.GetTargetLeaves(targetLeaves);

  m_alignment.reserve(targetLeaves.size());
  m_nonTermAlignment.resize(nonTermCount);

  for (int i = 0, j = 0; i < targetLeaves.size(); ++i) {
    const Node *leaf = targetLeaves[i];
    assert(leaf->GetType() != SOURCE);
    if (leaf->GetSpan().empty()) {
      continue;
    }
    std::map<const Node *, std::vector<int> >::iterator p =
      sinkToSourceIndices.find(leaf);
    assert(p != sinkToSourceIndices.end());
    std::vector<int> &sourceNodes = p->second;
    for (std::vector<int>::iterator r = sourceNodes.begin();
         r != sourceNodes.end(); ++r) {
      int srcIndex = *r;
      m_alignment.push_back(std::make_pair(srcIndex, i));
    }
    if (leaf->GetType() == TREE) {
      m_nonTermAlignment[nonTermSinkToSourceIndex[leaf]] = j++;
    }
  }
}

}  // namespace GHKM
}  // namespace Moses
