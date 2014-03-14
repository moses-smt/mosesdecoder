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
class Range;

class RuleElement
{
protected:
	const LatticeNode *m_latticeNode;
public:
	std::pair<size_t, size_t> m_alignmentPos;
	
	RuleElement(const RuleElement &copy);
	RuleElement(const LatticeNode &latticeNode)
	:m_latticeNode(&latticeNode)
	,m_alignmentPos(NOT_FOUND, NOT_FOUND)
	{}

	const LatticeNode &GetLatticeNode() const
	{ return *m_latticeNode; }

};

class Rule
{
protected:
	typedef std::vector<RuleElement> CollType;
	CollType m_coll;

	const LatticeNode *m_lhs;
	SymbolSequence m_source, m_target;
	
	bool IsHole(const TunnelCollection &tunnelColl) const;
	bool NonTermOverlap() const;

	const LatticeNode &GetLatticeNode(size_t ind) const;
	void CreateSymbols(const Global &global, bool &isValid, const SentenceAlignment &sentence);

public:
	// init
	Rule(const LatticeNode *latticeNode);

	// create new rule by appending node to prev rule
	Rule(const Rule &prevRule, const LatticeNode *latticeNode);

	// create copy with lhs
	Rule(const Global &global, bool &isValid, const Rule &copy, const LatticeNode *lhs, const SentenceAlignment &sentence);

	// can continue to add to this rule
	bool CanRecurse(const Global &global, const TunnelCollection &tunnelColl) const;

	virtual ~Rule();

	// can add this to the set of rules
	bool IsValid(const Global &global, const TunnelCollection &tunnelColl) const;

	size_t GetNumSymbols() const;
	bool AdjacentDefaultNonTerms() const;
	bool MaxNonTerm(const Global &global) const;
	bool MoreDefaultNonTermThanTerm() const;
	bool SourceHasEdgeDefaultNonTerm() const;

	void CreateRules(RuleCollection &rules
									 , const Lattice &lattice
									 , const SentenceAlignment &sentence
									 , const Global &global);
	
	int Compare(const Rule &compare) const;
	bool operator<(const Rule &compare) const;
			
	Range GetSourceRange() const;
	
	DEBUG_OUTPUT();

  void Output(std::ostream &out) const;
  void OutputInv(std::ostream &out) const;

};
