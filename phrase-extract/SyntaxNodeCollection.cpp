/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

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


#include "SyntaxNodeCollection.h"

#include <cassert>
#include <iostream>

namespace MosesTraining
{

SyntaxNodeCollection::~SyntaxNodeCollection()
{
  Clear();
}

void SyntaxNodeCollection::Clear()
{
  // loop through all m_nodes, delete them
  for(size_t i=0; i<m_nodes.size(); i++) {
    delete m_nodes[i];
  }
  m_nodes.clear();
  m_index.clear();
}

SyntaxNode *SyntaxNodeCollection::AddNode(int startPos, int endPos,
    const std::string &label)
{
  SyntaxNode* newNode = new SyntaxNode(label, startPos, endPos);
  m_nodes.push_back( newNode );
  m_index[ startPos ][ endPos ].push_back( newNode );
  m_endPositionsIndex[ endPos ].push_back( newNode );
  m_startPositionsIndex[ startPos ].push_back( newNode ); // TODO: may not need this: access m_index by startPos and iterate over its InnerNodeIndex (= end positions)?
  m_numWords = std::max(endPos+1, m_numWords);
  return newNode;
}

bool SyntaxNodeCollection::HasNode( int startPos, int endPos ) const
{
  return GetNodes( startPos, endPos).size() > 0;
}

const std::vector< SyntaxNode* >& SyntaxNodeCollection::GetNodes(
  int startPos, int endPos ) const
{
  NodeIndex::const_iterator startIndex = m_index.find( startPos );
  if (startIndex == m_index.end() )
    return m_emptyNode;

  InnerNodeIndex::const_iterator endIndex = startIndex->second.find( endPos );
  if (endIndex == startIndex->second.end())
    return m_emptyNode;

  return endIndex->second;
}

bool SyntaxNodeCollection::HasNodeStartingAtPosition( int startPos ) const
{
  return GetNodesByStartPosition(startPos).size() > 0;
}

const std::vector< SyntaxNode* >& SyntaxNodeCollection::GetNodesByStartPosition(
  int startPos ) const
{
  InnerNodeIndex::const_iterator startIndex = m_startPositionsIndex.find( startPos );
  if (startIndex == m_startPositionsIndex.end() )
    return m_emptyNode;

  return startIndex->second;
}

bool SyntaxNodeCollection::HasNodeEndingAtPosition( int endPos ) const
{
  return GetNodesByEndPosition(endPos).size() > 0;
}

const std::vector< SyntaxNode* >& SyntaxNodeCollection::GetNodesByEndPosition(
  int endPos ) const
{
  InnerNodeIndex::const_iterator endIndex = m_endPositionsIndex.find( endPos );
  if (endIndex == m_endPositionsIndex.end() )
    return m_emptyNode;

  return endIndex->second;
}

std::auto_ptr<SyntaxTree> SyntaxNodeCollection::ExtractTree()
{
  std::map<SyntaxNode *, SyntaxTree *> nodeToTree;

  // Create a SyntaxTree object for each SyntaxNode.
  for (std::vector<SyntaxNode*>::const_iterator p = m_nodes.begin();
       p != m_nodes.end(); ++p) {
    nodeToTree[*p] = new SyntaxTree(**p);
  }

  // Connect the SyntaxTrees.
  typedef NodeIndex::const_iterator OuterIterator;
  typedef InnerNodeIndex::const_reverse_iterator InnerIterator;

  SyntaxTree *root = 0;
  SyntaxNode *prevNode = 0;
  SyntaxTree *prevTree = 0;
  // Iterate over all start indices from lowest to highest.
  for (OuterIterator p = m_index.begin(); p != m_index.end(); ++p) {
    const InnerNodeIndex &inner = p->second;
    // Iterate over all end indices from highest to lowest.
    for (InnerIterator q = inner.rbegin(); q != inner.rend(); ++q) {
      const std::vector<SyntaxNode*> &nodes = q->second;
      // Iterate over all nodes that cover the same span in order of tree
      // depth, top-most first.
      for (std::vector<SyntaxNode*>::const_reverse_iterator r = nodes.rbegin();
           r != nodes.rend(); ++r) {
        SyntaxNode *node = *r;
        SyntaxTree *tree = nodeToTree[node];
        if (!prevNode) {
          // node is the root.
          root = tree;
          tree->parent() = 0;
        } else if (prevNode->start == node->start) {
          // prevNode is the parent of node.
          assert(prevNode->end >= node->end);
          tree->parent() = prevTree;
          prevTree->children().push_back(tree);
        } else {
          // prevNode is a descendant of node's parent.  The lowest common
          // ancestor of prevNode and node will be node's parent.
          SyntaxTree *ancestor = prevTree->parent();
          while (ancestor->value().end < tree->value().end) {
            ancestor = ancestor->parent();
          }
          assert(ancestor);
          tree->parent() = ancestor;
          ancestor->children().push_back(tree);
        }
        prevNode = node;
        prevTree = tree;
      }
    }
  }

  return std::auto_ptr<SyntaxTree>(root);
}

}  // namespace MosesTraining
