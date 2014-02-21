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


#include <iostream>
#include <cassert>
#include "SyntaxTree.h"
//#include "extract.h"
#include "Global.h"

//extern const Global g_debug;
extern const Global *g_global;

using namespace std;

bool SyntaxNode::IsSyntax() const
{
	bool ret = GetLabel() != "[X]";
	return ret;
}

SyntaxTree::SyntaxTree() 
:m_defaultLHS(0,0, "[X]")
{
	m_emptyNode.clear();
}

SyntaxTree::~SyntaxTree()
{
	// loop through all m_nodes, delete them
	for(int i=0; i<m_nodes.size(); i++)
	{
		delete m_nodes[i];
	}
}

bool HasDuplicates(const SyntaxNodes &nodes)
{
	string prevLabel;
	SyntaxNodes::const_iterator iter;
	for (iter = nodes.begin(); iter != nodes.end(); ++iter)
	{
		const SyntaxNode &node = **iter;
		string label = node.GetLabel();
		if (label == prevLabel)
			return true;
	}
	return false;
}

void SyntaxTree::AddNode( int startPos, int endPos, std::string label ) 
{	
	SyntaxNode* newNode = new SyntaxNode( startPos, endPos, "[" + label + "]");
	m_nodes.push_back( newNode );
	
	SyntaxNodes &nodesChart = m_index[ startPos ][ endPos ];
	
	if (!g_global->uppermostOnly)
	{
		nodesChart.push_back( newNode );	
		//assert(!HasDuplicates(m_index[ startPos ][ endPos ]));
	}
	else 
	{
		if (nodesChart.size() > 0)
		{
			assert(nodesChart.size() == 1);
			//delete nodes[0];
			nodesChart.resize(0);
		}
		assert(nodesChart.size() == 0);
		nodesChart.push_back( newNode );	
	}
}

ParentNodes SyntaxTree::Parse() {
	ParentNodes parents;

	int size = m_index.size();

	// looping through all spans of size >= 2
	for( int length=2; length<=size; length++ )
	{
		for( int startPos = 0; startPos <= size-length; startPos++ )
		{
			if (HasNode( startPos, startPos+length-1 ))
			{
				// processing one (parent) span

				//std::cerr << "# " << startPos << "-" << (startPos+length-1) << ":";
				SplitPoints splitPoints;
				splitPoints.push_back( startPos );
				//std::cerr << " " << startPos;

				int first = 1;
				int covered = 0;
				while( covered < length )
				{
					// find largest covering subspan (child)
					// starting at last covered position
					for( int midPos=length-first; midPos>covered; midPos-- )
					{
						if( HasNode( startPos+covered, startPos+midPos-1 ) )
						{							
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

const SyntaxNodes &SyntaxTree::GetNodes( int startPos, int endPos ) const 
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

void SyntaxTree::AddDefaultNonTerms(size_t phraseSize)
{
	for (size_t startPos = 0; startPos <= phraseSize; ++startPos)
	{
		for (size_t endPos = startPos; endPos < phraseSize; ++endPos)
		{
			AddNode(startPos, endPos, "X");
		}
	}
}

void SyntaxTree::AddDefaultNonTerms(bool isSyntax, bool mixed, size_t phraseSize)
{
	if (isSyntax)
	{
		AddDefaultNonTerms(!mixed, phraseSize);
	}
	else 
	{ // add X everywhere
		AddDefaultNonTerms(phraseSize);
	}
}

void SyntaxTree::AddDefaultNonTerms(bool addEverywhere, size_t phraseSize)
{
  //cerr << "GetNumWords()=" << GetNumWords() << endl;
	//assert(phraseSize == GetNumWords() || GetNumWords() == 1); // 1 if syntax sentence doesn't have any xml. TODO fix syntax tree obj

	for (size_t startPos = 0; startPos <= phraseSize; ++startPos)
	{
		for (size_t endPos = startPos; endPos <= phraseSize; ++endPos)
		{
			const SyntaxNodes &nodes = GetNodes(startPos, endPos);
			if (!addEverywhere && nodes.size() > 0)
			{ // only add if no label
				continue;
			}
			AddNode(startPos, endPos, "X");
		}
	}
}

const SyntaxNodes SyntaxTree::GetNodesForLHS( int startPos, int endPos ) const
{
	SyntaxNodes ret(GetNodes(startPos, endPos));
	
	if (ret.size() == 0)
		ret.push_back(&m_defaultLHS);
	
	return ret;
}

std::ostream& operator<<(std::ostream& os, const SyntaxTree& t)
{
	int size = t.m_index.size();
	for(size_t length=1; length<=size; length++)
	{
		for(size_t space=0; space<length; space++)
		{
			os << "    ";
		}
		for(size_t start=0; start<=size-length; start++)
		{
			
			if (t.HasNode( start, start+(length-1) ))
			{
				std::string label = t.GetNodes( start, start+(length-1) )[0]->GetLabel() + "#######";
				
				os << label.substr(0,7) << " ";
			}
			else
			{
				os << "------- ";
			}		
		}
		os << std::endl;
	}
  return os;
}


