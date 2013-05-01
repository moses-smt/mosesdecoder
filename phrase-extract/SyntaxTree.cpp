// $Id: SyntaxTree.cpp 1960 2008-12-15 12:52:38Z phkoehn $
// vim:tabstop=2

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


#include "SyntaxTree.h"

#include <cassert>
#include <iostream>

namespace MosesTraining
{

SyntaxTree::~SyntaxTree()
{
  Clear();
}

void SyntaxTree::Clear()
{
  m_top = 0;
  // loop through all m_nodes, delete them
  for(size_t i=0; i<m_nodes.size(); i++) {
    delete m_nodes[i];
  }
  m_nodes.clear();
  m_index.clear();
}

SyntaxNode *SyntaxTree::AddNode( int startPos, int endPos, std::string label )
{
  SyntaxNode* newNode = new SyntaxNode( startPos, endPos, label );
  m_nodes.push_back( newNode );
  m_index[ startPos ][ endPos ].push_back( newNode );
  m_size = std::max(endPos+1, m_size);
  return newNode;
}

ParentNodes SyntaxTree::Parse()
{
  ParentNodes parents;

  // looping through all spans of size >= 2
  for( int length=2; length<=m_size; length++ ) {
    for( int startPos = 0; startPos <= m_size-length; startPos++ ) {
      if (HasNode( startPos, startPos+length-1 )) {
        // processing one (parent) span

        //std::cerr << "# " << startPos << "-" << (startPos+length-1) << ":";
        SplitPoints splitPoints;
        splitPoints.push_back( startPos );
        //std::cerr << " " << startPos;

        int first = 1;
        int covered = 0;
        int found_somehing = 1; // break loop if nothing found
        while( covered < length && found_somehing ) {
          // find largest covering subspan (child)
          // starting at last covered position
          found_somehing = 0;
          for( int midPos=length-first; midPos>covered; midPos-- ) {
            if( HasNode( startPos+covered, startPos+midPos-1 ) ) {
              covered = midPos;
              splitPoints.push_back( startPos+covered );
              // std::cerr << " " << ( startPos+covered );
              first = 0;
              found_somehing = 1;
            }
          }
        }
        // std::cerr << std::endl;
        parents.push_back( splitPoints );
      }
    }
  }
  return parents;
}

bool SyntaxTree::HasNode( int startPos, int endPos ) const
{
  return GetNodes( startPos, endPos).size() > 0;
}

const std::vector< SyntaxNode* >& SyntaxTree::GetNodes( int startPos, int endPos ) const
{
  SyntaxTreeIndexIterator startIndex = m_index.find( startPos );
  if (startIndex == m_index.end() )
    return m_emptyNode;

  SyntaxTreeIndexIterator2 endIndex = startIndex->second.find( endPos );
  if (endIndex == startIndex->second.end())
    return m_emptyNode;

  return endIndex->second;
}

// for printing out tree
std::string SyntaxTree::ToString() const
{
  std::stringstream out;
  out << *this;
  return out.str();
}

void SyntaxTree::ConnectNodes()
{
  typedef SyntaxTreeIndex2::const_reverse_iterator InnerIterator;

  SyntaxNode *prev = 0;
  // Iterate over all start indices from lowest to highest.
  for (SyntaxTreeIndexIterator p = m_index.begin(); p != m_index.end(); ++p) {
    const SyntaxTreeIndex2 &inner = p->second;
    // Iterate over all end indices from highest to lowest.
    for (InnerIterator q = inner.rbegin(); q != inner.rend(); ++q) {
      const std::vector<SyntaxNode*> &nodes = q->second;
      // Iterate over all nodes that cover the same span in order of tree
      // depth, top-most first.
      for (std::vector<SyntaxNode*>::const_reverse_iterator r = nodes.rbegin();
           r != nodes.rend(); ++r) {
        SyntaxNode *node = *r;
        if (!prev) {
          // node is the root.
          m_top = node;
          node->SetParent(0);
        } else if (prev->GetStart() == node->GetStart()) {
          // prev is the parent of node.
          assert(prev->GetEnd() >= node->GetEnd());
          node->SetParent(prev);
          prev->AddChild(node);
        } else {
          // prev is a descendant of node's parent.  The lowest common
          // ancestor of prev and node will be node's parent.
          SyntaxNode *ancestor = prev->GetParent();
          while (ancestor->GetEnd() < node->GetEnd()) {
            ancestor = ancestor->GetParent();
          }
          assert(ancestor);
          node->SetParent(ancestor);
          ancestor->AddChild(node);
        }
        prev = node;
      }
    }
  }
}

std::ostream& operator<<(std::ostream& os, const SyntaxTree& t)
{
  size_t size = t.m_index.size();
  for(size_t length=1; length<=size; length++) {
    for(size_t space=0; space<length; space++) {
      os << "    ";
    }
    for(size_t start=0; start<=size-length; start++) {

      if (t.HasNode( start, start+(length-1) )) {
        std::string label = t.GetNodes( start, start+(length-1) )[0]->GetLabel() + "#######";

        os << label.substr(0,7) << " ";
      } else {
        os << "------- ";
      }
    }
    os << std::endl;
  }
  return os;
}

}

