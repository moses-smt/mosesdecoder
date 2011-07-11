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

class Tunnel;
class SyntaxNode;
class SentenceAlignment;
class SymbolSequence;

class LatticeNode
{
public:
	static size_t s_count;
	
	const Tunnel &m_hole;
	bool m_isTerminal;
	const SyntaxNode *m_sourceTreeNode, *m_targetTreeNode;
	const SentenceAlignment *m_sentence;
	
	// for terms
	LatticeNode(const Tunnel &hole, const SentenceAlignment *sentence);

	// for non-terms
	LatticeNode(const Tunnel &hole, const SyntaxNode *sourceTreeNode, const SyntaxNode *targetTreeNode);
	
	bool IsSyntax() const;
	
	size_t GetNumSymbols(size_t direction) const;
	
	std::string ToString(size_t direction) const;
	
	int Compare(const LatticeNode &otherNode) const;
	
	void CreateSymbols(size_t direction, SymbolSequence &symbols) const;

};

