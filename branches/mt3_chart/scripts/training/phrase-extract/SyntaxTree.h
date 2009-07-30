// $Id: SyntaxTree.h 1960 2008-12-15 12:52:38Z phkoehn $
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


#pragma once 
#include <string>
#include <vector>
#include <map>

class SyntaxNode {
protected:
	int m_start, m_end;
	std::string m_label;
	std::vector< SyntaxNode* > m_children;
	SyntaxNode* m_parent;
public:
SyntaxNode( int startPos, int endPos, std::string label )
	:m_start(startPos)
		,m_end(endPos)
		,m_label(label)
	{}
	int GetStart() const
	{ return m_start; }
	int GetEnd() const
	{ return m_end; }
	std::string GetLabel() const
	{ return m_label; }
};


class SyntaxTree {
protected:
	std::vector< SyntaxNode* > m_nodes;
	std::map< int, std::map< int, std::vector< SyntaxNode* > > > m_index;
	SyntaxNode* m_top;
	typedef std::map< int, std::map< int, std::vector< SyntaxNode* > > >::const_iterator SyntaxTreeIndexIterator;
	typedef std::map< int, std::vector< SyntaxNode* > >::const_iterator SyntaxTreeIndexIterator2;
	
public:
	SyntaxTree() {}
	~SyntaxTree() {
		// loop through all m_nodes, delete them
	}
	
	void AddNode( int startPos, int endPos, std::string label );
	void Parse();
	bool HasNode( int startPos, int endPos );
	const std::vector< SyntaxNode* >& GetNodes( int startPos, int endPos );
	const std::vector< SyntaxNode* >& GetAllNodes() 
  { return m_nodes; };
};


