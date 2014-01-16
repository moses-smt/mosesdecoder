/*
 *  Lattice.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 18/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <cassert>
#include "Lattice.h"
#include "LatticeNode.h"
#include "Tunnel.h"
#include "TunnelCollection.h"
#include "SyntaxTree.h"
#include "SentenceAlignment.h"
#include "tables-core.h"
#include "Rule.h"
#include "RuleCollection.h"

using namespace std;

Lattice::Lattice(size_t sourceSize)
:m_stacks(sourceSize + 1)
{
}

Lattice::~Lattice()
{
	std::vector<Stack>::iterator iterStack;
	for (iterStack = m_stacks.begin(); iterStack != m_stacks.end(); ++iterStack)
	{
		Stack &stack = *iterStack;
		RemoveAllInColl(stack);
	}	
}

void Lattice::CreateArcs(size_t startPos, const TunnelCollection &tunnelColl, const SentenceAlignment &sentence, const Global &global)
{	
	// term
	Stack &startStack = GetStack(startPos);
	
	LatticeNode *node = new LatticeNode(startPos, &sentence);
	startStack.push_back(node);
	
	// non-term
	for (size_t endPos = startPos + 1; endPos <= sentence.source.size(); ++endPos)
	{
		const TunnelList &tunnels = tunnelColl.GetTunnels(startPos, endPos - 1);
		
		TunnelList::const_iterator iterHole;
		for (iterHole = tunnels.begin(); iterHole != tunnels.end(); ++iterHole)
		{
			const Tunnel &tunnel = *iterHole;
			CreateArcsUsing1Hole(tunnel, sentence, global);
		}
	}
}

void Lattice::CreateArcsUsing1Hole(const Tunnel &tunnel, const SentenceAlignment &sentence, const Global &global)
{
	size_t startPos	= tunnel.GetRange(0).GetStartPos()
				, endPos	= tunnel.GetRange(0).GetEndPos();
	size_t numSymbols = tunnel.GetRange(0).GetWidth();
	assert(numSymbols > 0);
	
	Stack &startStack = GetStack(startPos);

		
	// non-terms. cartesian product of source & target labels
	assert(startPos == tunnel.GetRange(0).GetStartPos() && endPos == tunnel.GetRange(0).GetEndPos());
	size_t startT	= tunnel.GetRange(1).GetStartPos()
				,endT		= tunnel.GetRange(1).GetEndPos();
	
	const SyntaxNodes &nodesS = sentence.sourceTree.GetNodes(startPos, endPos);
	const SyntaxNodes &nodesT = sentence.targetTree.GetNodes(startT, endT );

	SyntaxNodes::const_iterator iterS, iterT;
	for (iterS = nodesS.begin(); iterS != nodesS.end(); ++iterS)
	{
		const SyntaxNode *syntaxNodeS = *iterS;
		
		for (iterT = nodesT.begin(); iterT != nodesT.end(); ++iterT)
		{
			const SyntaxNode *syntaxNodeT = *iterT;
			
			bool isSyntax = syntaxNodeS->IsSyntax() || syntaxNodeT->IsSyntax();
			size_t maxSourceNonTermSpan = isSyntax ? global.maxHoleSpanSourceSyntax : global.maxHoleSpanSourceDefault;
			
			if (maxSourceNonTermSpan >= endPos - startPos)
			{				
				LatticeNode *node = new LatticeNode(tunnel, syntaxNodeS, syntaxNodeT);
				startStack.push_back(node);
			}
		}
	}
}

Stack &Lattice::GetStack(size_t startPos)
{
	assert(startPos < m_stacks.size());
	return m_stacks[startPos];
}

const Stack &Lattice::GetStack(size_t startPos) const
{
	assert(startPos < m_stacks.size());
	return m_stacks[startPos];
}

void Lattice::CreateRules(size_t startPos, const SentenceAlignment &sentence, const Global &global)
{
	const Stack &startStack = GetStack(startPos);
	
	Stack::const_iterator iterStack;
	for (iterStack = startStack.begin(); iterStack != startStack.end(); ++iterStack)
	{
		const LatticeNode *node = *iterStack;
		Rule *initRule = new Rule(node);
		
		if (initRule->CanRecurse(global, sentence.GetTunnelCollection()))
		{ // may or maynot be valid, but can continue to build on this rule
			initRule->CreateRules(m_rules, *this, sentence, global);
		}

		if (initRule->IsValid(global, sentence.GetTunnelCollection()))
		{ // add to rule collection
			m_rules.Add(global, initRule, sentence);
		}
		else
		{
			delete initRule;
		}

		
	}
}

Stack Lattice::GetNonTermNode(const Range &sourceRange) const
{
	Stack ret;
	size_t sourcePos = sourceRange.GetStartPos();
	
	const Stack &origStack = GetStack(sourcePos);
	Stack::const_iterator iter;
	for (iter = origStack.begin(); iter != origStack.end(); ++iter)
	{
		LatticeNode *node = *iter;
		const Range &nodeRangeS = node->GetSourceRange();
		
		assert(nodeRangeS.GetStartPos() == sourceRange.GetStartPos());
		
		if (! node->IsTerminal() && nodeRangeS.GetEndPos() == sourceRange.GetEndPos())
		{
			ret.push_back(node);
		}
	}
	
	return ret;
}

std::ostream& operator<<(std::ostream &out, const Lattice &obj)
{
	std::vector<Stack>::const_iterator iter;
	for (iter = obj.m_stacks.begin(); iter != obj.m_stacks.end(); ++iter)
	{
		const Stack &stack = *iter;

		Stack::const_iterator iterStack;
		for (iterStack = stack.begin(); iterStack != stack.end(); ++iterStack)
		{
			const LatticeNode &node = **iterStack;
			out << node << " ";
		}
	}

	return out;
}


