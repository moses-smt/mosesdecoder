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

void Lattice::CreateArcs(size_t startPos, const TunnelCollection &holeColl, const SentenceAlignment &sentence, const Global &global)
{	
	for (size_t endPos = startPos + 1; endPos <= sentence.source.size(); ++endPos)
	{
		const TunnelList &holes = holeColl.GetHoles(startPos, endPos - 1);
		
		TunnelList::const_iterator iterHole;
		for (iterHole = holes.begin(); iterHole != holes.end(); ++iterHole)
		{
			const Tunnel &hole = *iterHole;
			CreateArcsUsing1Hole(hole, sentence, global);
		}
	}
}

void Lattice::CreateArcsUsing1Hole(const Tunnel &hole, const SentenceAlignment &sentence, const Global &global)
{
	size_t startPos	= hole.GetStart(0)
				, endPos	= hole.GetEnd(0) + 1; // lattice indexing
	size_t numSymbols = endPos - startPos;
	
	Stack &startStack = GetStack(startPos);

	// do terminals 1st
	assert(numSymbols > 0);
	
	if (numSymbols <= global.maxSymbolsSource)
	{
		LatticeNode *node = new LatticeNode(hole, &sentence);
		startStack.push_back(node);
	}
	
	// non-terms. cartesian product of source & target labels
	assert(startPos == hole.GetStart(0) && endPos - 1 == hole.GetEnd(0));
	size_t startT	= hole.GetStart(1)
				,endT		= hole.GetEnd(1);
	
	const SyntaxNodes &nodesS = sentence.sourceTree.GetNodes(startPos, endPos - 1);
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
				LatticeNode *node = new LatticeNode(hole, syntaxNodeS, syntaxNodeT);
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
		
		if (initRule->CanRecurse(global, *sentence.holeCollection))
		{ // may or maynot be valid, but can continue to build on this rule
			initRule->CreateRules(m_rules, *this, sentence, global);
		}

		if (initRule->IsValid(global, *sentence.holeCollection))
		{ // add to rule collection
			m_rules.Add(initRule, sentence);
		}
		else
		{
			delete initRule;
		}

		
	}
}


