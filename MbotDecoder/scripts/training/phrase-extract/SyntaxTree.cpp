// $Id: SyntaxTree.cpp,v 1.1.1.1 2013/01/06 16:54:11 braunefe Exp $
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
#include <iostream>

SyntaxTree::~SyntaxTree()
{
  // loop through all m_nodes, delete them
  for(int i=0; i<m_nodes.size(); i++) {
    delete m_nodes[i];
  }
}

void SyntaxTree::AddNode( int startPos, int endPos, std::string label )
{
  SyntaxNode* newNode = new SyntaxNode( startPos, endPos, label );
  m_nodes.push_back( newNode );
  m_index[ startPos ][ endPos ].push_back( newNode );
}

ParentNodes SyntaxTree::Parse()
{
  ParentNodes parents;

  int size = m_index.size();

  // looping through all spans of size >= 2
  for( int length=2; length<=size; length++ ) {
    for( int startPos = 0; startPos <= size-length; startPos++ ) {
      if (HasNode( startPos, startPos+length-1 )) {
        // processing one (parent) span

        //std::cerr << "# " << startPos << "-" << (startPos+length-1) << ":";
        SplitPoints splitPoints;
        splitPoints.push_back( startPos );
        //std::cerr << " " << startPos;

        int first = 1;
        int covered = 0;
        while( covered < length ) {
          // find largest covering subspan (child)
          // starting at last covered position
          for( int midPos=length-first; midPos>covered; midPos-- ) {
            if( HasNode( startPos+covered, startPos+midPos-1 ) ) {
              covered = midPos;
              splitPoints.push_back( startPos+covered );
              // std::cerr << " " << ( startPos+covered );
              first = 0;
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

std::ostream& operator<<(std::ostream& os, const SyntaxTree& t)
{
  int size = t.m_index.size();
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


