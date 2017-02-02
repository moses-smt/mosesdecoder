#include "HyperTree.h"

namespace Moses
{
namespace Syntax
{
namespace F2S
{

void HyperTree::Node::Prune(std::size_t tableLimit)
{
  // Recusively prune child nodes.
  for (Map::iterator p = m_map.begin(); p != m_map.end(); ++p) {
    p->second.Prune(tableLimit);
  }
  // Prune TargetPhraseCollection at this node.
  m_targetPhraseCollection->Prune(true, tableLimit);
}

void HyperTree::Node::Sort(std::size_t tableLimit)
{
  // Recusively sort child nodes.
  for (Map::iterator p = m_map.begin(); p != m_map.end(); ++p) {
    p->second.Sort(tableLimit);
  }
  // Sort TargetPhraseCollection at this node.
  m_targetPhraseCollection->Sort(true, tableLimit);
}

HyperTree::Node *HyperTree::Node::GetOrCreateChild(
  const HyperPath::NodeSeq &nodeSeq)
{
  return &m_map[nodeSeq];
}

const HyperTree::Node *HyperTree::Node::GetChild(
  const HyperPath::NodeSeq &nodeSeq) const
{
  Map::const_iterator p = m_map.find(nodeSeq);
  return (p == m_map.end()) ? NULL : &p->second;
}

TargetPhraseCollection::shared_ptr HyperTree::GetOrCreateTargetPhraseCollection(
  const HyperPath &hyperPath)
{
  Node &node = GetOrCreateNode(hyperPath);
  return node.GetTargetPhraseCollection();
}

HyperTree::Node &HyperTree::GetOrCreateNode(const HyperPath &hyperPath)
{
  const std::size_t height = hyperPath.nodeSeqs.size();
  Node *node = &m_root;
  for (std::size_t i = 0; i < height; ++i) {
    const HyperPath::NodeSeq &nodeSeq = hyperPath.nodeSeqs[i];
    node = node->GetOrCreateChild(nodeSeq);
  }
  return *node;
}

void HyperTree::SortAndPrune(std::size_t tableLimit)
{
  if (tableLimit) {
    m_root.Sort(tableLimit);
  }
}

}  // namespace F2S
}  // namespace Syntax
}  // namespace Moses
