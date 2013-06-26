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

#include "ScfgRule.h"

#include "Node.h"
#include "Subgraph.h"

#include <algorithm>

namespace Moses
{
namespace GHKM
{

ScfgRule::ScfgRule(const Subgraph &fragment)
  : m_sourceLHS("X", NonTerminal)
  , m_targetLHS(fragment.GetRoot()->GetLabel(), NonTerminal)
  , m_pcfgScore(fragment.GetPcfgScore())
{
  // Source RHS

  const std::set<const Node *> &leaves = fragment.GetLeaves();

  std::vector<const Node *> sourceRHSNodes;
  sourceRHSNodes.reserve(leaves.size());
  for (std::set<const Node *>::const_iterator p(leaves.begin());
       p != leaves.end(); ++p) {
    const Node &leaf = **p;
    if (!leaf.GetSpan().empty()) {
      sourceRHSNodes.push_back(&leaf);
    }
  }

  std::sort(sourceRHSNodes.begin(), sourceRHSNodes.end(), PartitionOrderComp);

  // Build a mapping from target nodes to source-order indices, so that we
  // can construct the Alignment object later.
  std::map<const Node *, std::vector<int> > sourceOrder;

  m_sourceRHS.reserve(sourceRHSNodes.size());
  int srcIndex = 0;
  for (std::vector<const Node *>::const_iterator p(sourceRHSNodes.begin());
       p != sourceRHSNodes.end(); ++p, ++srcIndex) {
    const Node &sinkNode = **p;
    if (sinkNode.GetType() == TREE) {
      m_sourceRHS.push_back(Symbol("X", NonTerminal));
      sourceOrder[&sinkNode].push_back(srcIndex);
    } else {
      assert(sinkNode.GetType() == SOURCE);
      m_sourceRHS.push_back(Symbol(sinkNode.GetLabel(), Terminal));
      // Add all aligned target words to the sourceOrder map
      const std::vector<Node *> &parents(sinkNode.GetParents());
      for (std::vector<Node *>::const_iterator q(parents.begin());
           q != parents.end(); ++q) {
        if ((*q)->GetType() == TARGET) {
          sourceOrder[*q].push_back(srcIndex);
        }
      }
    }
  }

  // Target RHS + alignment

  std::vector<const Node *> targetLeaves;
  fragment.GetTargetLeaves(targetLeaves);

  m_alignment.reserve(targetLeaves.size());  // might be too much but that's OK
  m_targetRHS.reserve(targetLeaves.size());

  for (std::vector<const Node *>::const_iterator p(targetLeaves.begin());
       p != targetLeaves.end(); ++p) {
    const Node &leaf = **p;
    if (leaf.GetSpan().empty()) {
      // The node doesn't cover any source words, so we can only add
      // terminals to the target RHS (not a non-terminal).
      std::vector<std::string> targetWords(leaf.GetTargetWords());
      for (std::vector<std::string>::const_iterator q(targetWords.begin());
           q != targetWords.end(); ++q) {
        m_targetRHS.push_back(Symbol(*q, Terminal));
      }
    } else if (leaf.GetType() == SOURCE) {
      // Do nothing
    } else {
      SymbolType type = (leaf.GetType() == TREE) ? NonTerminal : Terminal;
      m_targetRHS.push_back(Symbol(leaf.GetLabel(), type));

      int tgtIndex = m_targetRHS.size()-1;
      std::map<const Node *, std::vector<int> >::iterator q(sourceOrder.find(&leaf));
      assert(q != sourceOrder.end());
      std::vector<int> &sourceNodes = q->second;
      for (std::vector<int>::iterator r(sourceNodes.begin());
           r != sourceNodes.end(); ++r) {
        int srcIndex = *r;
        m_alignment.push_back(std::make_pair(srcIndex, tgtIndex));
      }
    }
  }
}

int ScfgRule::Scope() const
{
  int scope = 0;
  bool predIsNonTerm = false;
  if (m_sourceRHS[0].GetType() == NonTerminal) {
    ++scope;
    predIsNonTerm = true;
  }
  for (size_t i = 1; i < m_sourceRHS.size(); ++i) {
    bool isNonTerm = m_sourceRHS[i].GetType() == NonTerminal;
    if (isNonTerm && predIsNonTerm) {
      ++scope;
    }
    predIsNonTerm = isNonTerm;
  }
  if (predIsNonTerm) {
    ++scope;
  }
  return scope;
}

bool ScfgRule::PartitionOrderComp(const Node *a, const Node *b)
{
  const Span &aSpan = a->GetSpan();
  const Span &bSpan = b->GetSpan();
  assert(!aSpan.empty() && !bSpan.empty());
  return *(aSpan.begin()) < *(bSpan.begin());
}

}  // namespace GHKM
}  // namespace Moses
