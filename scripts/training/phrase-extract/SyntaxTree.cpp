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
#include <iostream>

void SyntaxTree::AddNode( int startPos, int endPos, std::string label ) {
	SyntaxNode* newNode = new SyntaxNode( startPos, endPos, label );
	m_nodes.push_back( newNode );
	m_index[ startPos ][ endPos ].push_back( newNode );
}

void SyntaxTree::Parse() {
}

bool SyntaxTree::HasNode( int startPos, int endPos ) {
	return m_index[ startPos ][ endPos ].size() > 0;
}

const std::vector< SyntaxNode* >& SyntaxTree::GetNodes( int startPos, int endPos ) {
	return m_index[ startPos ][ endPos ];
}
