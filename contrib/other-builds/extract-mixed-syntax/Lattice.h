#pragma once
/*
 *  Lattice.h
 *  extract
 *
 *  Created by Hieu Hoang on 18/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <iostream>
#include <vector>
#include "RuleCollection.h"

class Global;
class LatticeNode;
class Tunnel;
class TunnelCollection;
class SentenceAlignment;

typedef std::vector<LatticeNode*> Stack;

class Lattice
{
	friend std::ostream& operator<<(std::ostream&, const Lattice&);

	std::vector<Stack> m_stacks;
	RuleCollection m_rules;
	
	Stack &GetStack(size_t endPos);			
	
	void CreateArcsUsing1Hole(const Tunnel &tunnel, const SentenceAlignment &sentence, const Global &global);

public:
	Lattice(size_t sourceSize);
	~Lattice();
	
	void CreateArcs(size_t startPos, const TunnelCollection &tunnelColl, const SentenceAlignment &sentence, const Global &global);
	void CreateRules(size_t startPos, const SentenceAlignment &sentence, const Global &global);

	const Stack &GetStack(size_t startPos) const;			
	const RuleCollection &GetRules() const
	{ return m_rules; }
	
	Stack GetNonTermNode(const Range &sourceRange) const;			

};

