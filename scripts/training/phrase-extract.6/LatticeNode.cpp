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
LatticeNode::LatticeNode(const Tunnel &hole, const SentenceAlignment *sentence)
:m_hole(hole)
,m_isTerminal(true)
,m_sourceTreeNode(NULL)
,m_targetTreeNode(NULL)
,m_sentence(sentence)
{
	s_count++;
}

// for non-terms
LatticeNode::LatticeNode(const Tunnel &hole, const SyntaxNode *sourceTreeNode, const SyntaxNode *targetTreeNode)
:m_hole(hole)
,m_isTerminal(false)
,m_sourceTreeNode(sourceTreeNode)
,m_targetTreeNode(targetTreeNode)
,m_sentence(NULL)
{
	s_count++;
}

bool LatticeNode::IsSyntax() const
{
	assert(!m_isTerminal);
	bool ret = m_sourceTreeNode->IsSyntax() || m_targetTreeNode->IsSyntax();
	return ret;
}

std::string LatticeNode::ToString(size_t direction) const
{
	stringstream ret;
	
	if (m_isTerminal)
	{
		const std::vector<std::string> &words = (direction == 0 ? m_sentence->source : m_sentence->target);
		size_t startPos = m_hole.GetStart(direction)
					,endPos = m_hole.GetEnd(direction);
		
		for (size_t pos = startPos; pos <= endPos; ++pos)
			ret << words[pos] << " ";
	}
	else
	{ // output both
		ret << m_sourceTreeNode->GetLabel() << m_targetTreeNode->GetLabel() << " "; 
	}

	return ret.str();
}

size_t LatticeNode::GetNumSymbols(size_t direction) const
{
	if (m_isTerminal)
	{
		size_t ret = (direction == 0) ? m_hole.GetSpan(0) : m_hole.GetSpan(1);
		return ret;
	}
	else 
	{
		return 1;
	}

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
	{ // term. compare span
		ret = m_hole.Compare(otherNode.m_hole);
	}
	else
	{ // non-term. compare labels & span
		assert(m_isTerminal == false);
		assert(m_isTerminal == otherNode.m_isTerminal);

		if (m_sourceTreeNode->IsSyntax())
		{
			ret = m_hole.Compare(otherNode.m_hole, 0);
			if (ret == 0 && m_sourceTreeNode->GetLabel() != otherNode.m_sourceTreeNode->GetLabel())
			{
				ret = (m_sourceTreeNode->GetLabel() < otherNode.m_sourceTreeNode->GetLabel()) ? -1 : +1;
			}			
		}

		if (ret == 0 && m_targetTreeNode->IsSyntax())
		{
			ret = m_hole.Compare(otherNode.m_hole, 1);
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
		const std::vector<std::string> &words = (direction == 0 ? m_sentence->source : m_sentence->target);
		size_t startPos = m_hole.GetStart(direction)
						,endPos = m_hole.GetEnd(direction);
		
		for (size_t pos = startPos; pos <= endPos; ++pos)
		{
			Symbol symbol(words[pos], pos);
			symbols.Add(symbol);
		}
	}
	else
	{ // output both
		
		Symbol symbol(m_sourceTreeNode->GetLabel(), m_targetTreeNode->GetLabel()
									, m_hole.GetStart(0), m_hole.GetEnd(0)
									, m_hole.GetStart(1), m_hole.GetEnd(1)
									, m_sourceTreeNode->IsSyntax(), m_targetTreeNode->IsSyntax());

		symbols.Add(symbol);
	}
	
}


