#pragma once
/*
 *  LatticeNode.h
 *  extract
 *
 *  Created by Hieu Hoang on 18/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <iostream>
#include <cassert>
#include "Range.h"

class Tunnel;
class SyntaxNode;
class SentenceAlignment;
class SymbolSequence;

class LatticeNode
{
	friend std::ostream& operator<<(std::ostream&, const LatticeNode&);

	bool m_isTerminal;

	// for terms & non-term
	Range m_sourceRange;

	// non-terms. source range should be same as m_sourceRange
	const Tunnel *m_tunnel;

public:
	static size_t s_count;
	
	
	
	const SyntaxNode *m_sourceTreeNode, *m_targetTreeNode;
	const SentenceAlignment *m_sentence;
	
	// for terms
	LatticeNode(size_t pos, const SentenceAlignment *sentence);

	// for non-terms
	LatticeNode(const Tunnel &tunnel, const SyntaxNode *sourceTreeNode, const SyntaxNode *targetTreeNode);
	
	bool IsTerminal() const
	{ return m_isTerminal; }

	bool IsSyntax() const;
	
	size_t GetNumSymbols(size_t direction) const;
	
	std::string ToString() const;
	
	int Compare(const LatticeNode &otherNode) const;
	
	void CreateSymbols(size_t direction, SymbolSequence &symbols) const;

	const Tunnel &GetTunnel() const
	{
		assert(m_tunnel);
		return *m_tunnel;
	}
	
	const Range &GetSourceRange() const
	{
		return m_sourceRange;
	}
	const SyntaxNode &GetSyntaxNode(size_t direction) const
	{
		const SyntaxNode *node = direction == 0 ? m_sourceTreeNode : m_targetTreeNode;
		assert(node);
		return *node;
	}
	
};

