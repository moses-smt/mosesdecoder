#pragma once
/*
 *  Rule.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include <iostream>
#include "LatticeNode.h"
#include "SymbolSequence.h"
#include "Global.h"

class Lattice;
class SentenceAlignment;
class Global;
class RuleCollection;
class SyntaxNode;
class TunnelCollection;

class RuleElement
{
protected:
	const LatticeNode *m_latticeNode;
public:
	size_t symbolPos[2];
	
	RuleElement(const RuleElement &copy);
	RuleElement(const LatticeNode &latticeNode, size_t sourcePos);

	const LatticeNode &GetLatticeNode() const
	{ return *m_latticeNode; }

};

class Rule
{
	friend std::ostream& operator<<(std::ostream &out, const Rule &obj);

protected:
	typedef std::vector<RuleElement> CollType;
	CollType m_coll;

	typedef std::vector<RuleElement*> CollTypeTarget;
	CollTypeTarget m_targetSorted;

	const SyntaxNode *m_lhsS, *m_lhsT;
	SymbolSequence m_source, m_target;
	
	void SortTargetLHS();
	bool IsHole(const TunnelCollection &holeColl) const;

	const LatticeNode &GetLatticeNode(size_t ind) const;
	void CreateSymbolsInterna();

public:
	// init
	Rule(const LatticeNode *latticeNode);

	// create new rule by appending node to prev rule
	Rule(const Rule &prevRule, const LatticeNode *latticeNode);

	// create copy with lhs
	Rule(const Rule &copy, const SyntaxNode *lhsS, const SyntaxNode *lhsT);

	// can continue to add to this rule
	bool CanRecurse(const Global &global, const TunnelCollection &holeColl) const;

	virtual ~Rule();

	// can add this to the set of rules
	bool IsValid(const Global &global, const TunnelCollection &holeColl) const;

	size_t GetNumSymbols() const;
	bool AdjacentDefaultNonTerms() const;
	bool MaxNonTerm(const Global &global) const;

	void CreateRules(RuleCollection &rules
									 , const Lattice &lattice
									 , const SentenceAlignment &sentence
									 , const Global &global);

	std::pair<size_t, size_t> GetSpan(size_t direction) const;
	
	int Compare(const Rule &compare) const;
	bool operator<(const Rule &compare) const;
	
	std::string ToString() const;
	
	void CreateSymbols();
	
	DEBUG_OUTPUT();
};
