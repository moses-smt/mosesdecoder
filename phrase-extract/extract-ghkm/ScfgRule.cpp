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
#include "SyntaxTree.h"

#include <algorithm>

namespace Moses
{
namespace GHKM
{

ScfgRule::ScfgRule(const Subgraph &fragment,
                   const MosesTraining::SyntaxTree *sourceSyntaxTree)
  : m_sourceLHS("X", NonTerminal)
  , m_targetLHS(fragment.GetRoot()->GetLabel(), NonTerminal)
  , m_pcfgScore(fragment.GetPcfgScore())
  , m_hasSourceLabels(sourceSyntaxTree)
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
  m_numberOfNonTerminals = 0;
  int srcIndex = 0;
  for (std::vector<const Node *>::const_iterator p(sourceRHSNodes.begin());
       p != sourceRHSNodes.end(); ++p, ++srcIndex) {
    const Node &sinkNode = **p;
    if (sinkNode.GetType() == TREE) {
      m_sourceRHS.push_back(Symbol("X", NonTerminal));
      sourceOrder[&sinkNode].push_back(srcIndex);
      ++m_numberOfNonTerminals;
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
    if (sourceSyntaxTree) {
      // Source syntax label
      PushSourceLabel(sourceSyntaxTree,&sinkNode,"XRHS");
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

  if (sourceSyntaxTree) {
    // Source syntax label for root node (if sourceSyntaxTree available)
    PushSourceLabel(sourceSyntaxTree,fragment.GetRoot(),"XLHS");
    // All non-terminal spans (including the LHS) should have obtained a label
    // (a source-side syntactic constituent label if the span matches, "XLHS" otherwise)
//    assert(m_sourceLabels.size() == m_numberOfNonTerminals+1);
  }
}

void ScfgRule::PushSourceLabel(const MosesTraining::SyntaxTree *sourceSyntaxTree,
                               const Node *node,
                               const std::string &nonMatchingLabel)
{
  ContiguousSpan span = Closure(node->GetSpan());
  if (sourceSyntaxTree->HasNode(span.first,span.second)) { // does a source constituent match the span?
    std::vector<MosesTraining::SyntaxNode*> sourceLabels =
      sourceSyntaxTree->GetNodes(span.first,span.second);
    if (!sourceLabels.empty()) {
      // store the topmost matching label from the source syntax tree
      m_sourceLabels.push_back(sourceLabels.back()->GetLabel());
    }
  } else {
    // no matching source-side syntactic constituent: store nonMatchingLabel
    m_sourceLabels.push_back(nonMatchingLabel);
  }
}

// TODO: rather implement the method external to ScfgRule
void ScfgRule::UpdateSourceLabelCoocCounts(std::map< std::string, std::map<std::string,float>* > &coocCounts, float count) const
{
  std::map<int, int> sourceToTargetNTMap;
  std::map<int, int> targetToSourceNTMap;

  for (Alignment::const_iterator p(m_alignment.begin());
       p != m_alignment.end(); ++p) {
    if ( m_sourceRHS[p->first].GetType() == NonTerminal ) {
      assert(m_targetRHS[p->second].GetType() == NonTerminal);
      sourceToTargetNTMap[p->first] = p->second;
    }
  }

  size_t sourceIndex = 0;
  size_t sourceNonTerminalIndex = 0;
  for (std::vector<Symbol>::const_iterator p=m_sourceRHS.begin();
       p != m_sourceRHS.end(); ++p, ++sourceIndex) {
    if ( p->GetType() == NonTerminal ) {
      const std::string &sourceLabel = m_sourceLabels[sourceNonTerminalIndex];
      int targetIndex = sourceToTargetNTMap[sourceIndex];
      const std::string &targetLabel = m_targetRHS[targetIndex].GetValue();
      ++sourceNonTerminalIndex;

      std::map<std::string,float>* countMap = NULL;
      std::map< std::string, std::map<std::string,float>* >::iterator iter = coocCounts.find(sourceLabel);
      if ( iter == coocCounts.end() ) {
        std::map<std::string,float> *newCountMap = new std::map<std::string,float>();
        std::pair< std::map< std::string, std::map<std::string,float>* >::iterator, bool > inserted =
          coocCounts.insert( std::pair< std::string, std::map<std::string,float>* >(sourceLabel, newCountMap) );
        assert(inserted.second);
        countMap = (inserted.first)->second;
      } else {
        countMap = iter->second;
      }
      std::pair< std::map<std::string,float>::iterator, bool > inserted =
        countMap->insert( std::pair< std::string,float>(targetLabel, count) );
      if ( !inserted.second ) {
        (inserted.first)->second += count;
      }
    }
  }
}

}  // namespace GHKM
}  // namespace Moses
