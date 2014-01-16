/*
 *  LatticeNode.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 18/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <sstream>
#include "LatticeNode.h"
#include "SyntaxTree.h"
#include "Tunnel.h"
#include "SentenceAlignment.h"
#include "SymbolSequence.h"

size_t LatticeNode::s_count = 0;

using namespace std;

// for terms
LatticeNode::LatticeNode(size_t pos, const SentenceAlignment *sentence)
:m_tunnel(NULL)
,m_isTerminal(true)
,m_sourceTreeNode(NULL)
,m_targetTreeNode(NULL)
,m_sentence(sentence)
,m_sourceRange(pos, pos)
{
	s_count++;
	//cerr << *this << endl;
}

// for non-terms
LatticeNode::LatticeNode(const Tunnel &tunnel, const SyntaxNode *sourceTreeNode, const SyntaxNode *targetTreeNode)
:m_tunnel(&tunnel)
,m_isTerminal(false)
,m_sourceTreeNode(sourceTreeNode)
,m_targetTreeNode(targetTreeNode)
,m_sentence(NULL)
,m_sourceRange(tunnel.GetRange(0))
{
	s_count++;
	//cerr << *this << endl;
}

bool LatticeNode::IsSyntax() const
{
	assert(!m_isTerminal);
	bool ret = m_sourceTreeNode->IsSyntax() || m_targetTreeNode->IsSyntax();
	return ret;
}

size_t LatticeNode::GetNumSymbols(size_t direction) const
{
	return 1;
}

int LatticeNode::Compare(const LatticeNode &otherNode) const
{
	int ret = 0;
	if (m_isTerminal != otherNode.m_isTerminal)
	{
		ret = m_isTerminal ? -1 : 1;
	}
	
	// both term or non-term
	else if (m_isTerminal)
	{ // term. compare source span
		if (m_sourceRange.GetStartPos() == otherNode.m_sourceRange.GetStartPos())
			ret = 0;
		else 
			ret = (m_sourceRange.GetStartPos() < otherNode.m_sourceRange.GetStartPos()) ? -1 : +1;
	}
	else
	{ // non-term. compare source span and BOTH label
		assert(!m_isTerminal);
		assert(!otherNode.m_isTerminal);

		if (m_sourceTreeNode->IsSyntax())
		{
			ret = m_tunnel->Compare(*otherNode.m_tunnel, 0);
			if (ret == 0 && m_sourceTreeNode->GetLabel() != otherNode.m_sourceTreeNode->GetLabel())
			{
				ret = (m_sourceTreeNode->GetLabel() < otherNode.m_sourceTreeNode->GetLabel()) ? -1 : +1;
			}			
		}

		if (ret == 0 && m_targetTreeNode->IsSyntax())
		{
			ret = m_tunnel->Compare(*otherNode.m_tunnel, 1);
			if (ret == 0 && m_targetTreeNode->GetLabel() != otherNode.m_targetTreeNode->GetLabel())
			{
				ret = (m_targetTreeNode->GetLabel() < otherNode.m_targetTreeNode->GetLabel()) ? -1 : +1;
			}
		}
	}
	
	return ret;
}

void LatticeNode::CreateSymbols(size_t direction, SymbolSequence &symbols) const
{
	if (m_isTerminal)
	{
		/*
		const std::vector<std::string> &words = (direction == 0 ? m_sentence->source : m_sentence->target);
		size_t startPos = m_tunnel.GetStart(direction)
						,endPos = m_tunnel.GetEnd(direction);
		
		for (size_t pos = startPos; pos <= endPos; ++pos)
		{
			Symbol symbol(words[pos], pos);
			symbols.Add(symbol);
		}
		 */
	}
	else
	{ // output both
		
		Symbol symbol(m_sourceTreeNode->GetLabel(), m_targetTreeNode->GetLabel()
									, m_tunnel->GetRange(0).GetStartPos(), m_tunnel->GetRange(0).GetEndPos()
									, m_tunnel->GetRange(1).GetStartPos(), m_tunnel->GetRange(1).GetEndPos()
									, m_sourceTreeNode->IsSyntax(), m_targetTreeNode->IsSyntax());

		symbols.Add(symbol);
	}
	
}

std::ostream& operator<<(std::ostream &out, const LatticeNode &obj)
{	
	if (obj.m_isTerminal)
	{
		assert(obj.m_sourceRange.GetWidth() == 1);
		size_t pos = obj.m_sourceRange.GetStartPos();
		
		const SentenceAlignment &sentence = *obj.m_sentence;
		out << obj.m_sourceRange << "=" << sentence.source[pos];		
	}
	else
	{ 
		assert(obj.m_tunnel);
		out << obj.GetTunnel() << "=" << obj.m_sourceTreeNode->GetLabel() << obj.m_targetTreeNode->GetLabel() << " "; 
	}
	
	return out;
}


