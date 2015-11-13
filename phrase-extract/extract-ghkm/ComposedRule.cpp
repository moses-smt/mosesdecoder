/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "ComposedRule.h"

#include <set>
#include <vector>
#include <queue>

#include "Node.h"
#include "Options.h"
#include "Subgraph.h"

namespace MosesTraining
{
namespace Syntax
{
namespace GHKM
{

ComposedRule::ComposedRule(const Subgraph &baseRule)
  : m_baseRule(baseRule)
  , m_depth(baseRule.GetDepth())
  , m_size(baseRule.GetSize())
  , m_nodeCount(baseRule.GetNodeCount())
{
  const std::set<const Node *> &leaves = baseRule.GetLeaves();
  for (std::set<const Node *>::const_iterator p = leaves.begin();
       p != leaves.end(); ++p) {
    if ((*p)->GetType() == TREE) {
      m_openAttachmentPoints.push(*p);
    }
  }
}

ComposedRule::ComposedRule(const ComposedRule &other, const Subgraph &rule,
                           int depth)
  : m_baseRule(other.m_baseRule)
  , m_attachedRules(other.m_attachedRules)
  , m_openAttachmentPoints(other.m_openAttachmentPoints)
  , m_depth(depth)
  , m_size(other.m_size+rule.GetSize())
  , m_nodeCount(other.m_nodeCount+rule.GetNodeCount()-1)
{
  m_attachedRules.push_back(&rule);
  m_openAttachmentPoints.pop();
}

const Node *ComposedRule::GetOpenAttachmentPoint()
{
  return m_openAttachmentPoints.empty() ? 0 : m_openAttachmentPoints.front();
}

void ComposedRule::CloseAttachmentPoint()
{
  assert(!m_openAttachmentPoints.empty());
  m_attachedRules.push_back(0);
  m_openAttachmentPoints.pop();
}

ComposedRule *ComposedRule::AttemptComposition(const Subgraph &rule,
    const Options &options) const
{
  // The smallest possible rule fragment should be rooted at a tree node.
  // Note that this differs from the original GHKM definition.
  assert(rule.GetRoot()->GetType() == TREE);

  // Check the node count of the proposed rule.
  if (m_nodeCount+rule.GetNodeCount()-1 > options.maxNodes) {
    return 0;
  }

  // Check the size of the proposed rule.
  if (m_size+rule.GetSize() > options.maxRuleSize) {
    return 0;
  }

  // Determine the depth of the proposed rule and test whether it exceeds the
  // limit.
  int attachmentPointDepth = 0;
  const Node *n = rule.GetRoot();
  while (n != m_baseRule.GetRoot()) {
    assert(n->GetParents().size() == 1);
    n = n->GetParents()[0];
    ++attachmentPointDepth;
  }
  int newDepth = std::max(m_depth, attachmentPointDepth+rule.GetDepth());
  if (newDepth > options.maxRuleDepth) {
    return 0;
  }

  return new ComposedRule(*this, rule, newDepth);
}

Subgraph ComposedRule::CreateSubgraph()
{
  std::set<const Node *> leaves;
  const std::set<const Node *> &baseLeaves = m_baseRule.GetLeaves();
  size_t i = 0;
  for (std::set<const Node *>::const_iterator p = baseLeaves.begin();
       p != baseLeaves.end(); ++p) {
    const Node *baseLeaf = *p;
    if (baseLeaf->GetType() == TREE && i < m_attachedRules.size()) {
      const Subgraph *attachedRule = m_attachedRules[i++];
      if (attachedRule) {
        leaves.insert(attachedRule->GetLeaves().begin(),
                      attachedRule->GetLeaves().end());
        continue;
      }
    }
    leaves.insert(baseLeaf);
  }
  return Subgraph(m_baseRule.GetRoot(), leaves);
}

}  // namespace GHKM
}  // namespace Syntax
}  // namespace MosesTraining
