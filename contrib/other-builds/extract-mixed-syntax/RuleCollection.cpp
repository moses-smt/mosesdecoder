/*
 *  RuleCollection.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "RuleCollection.h"
#include "Rule.h"
#include "SentenceAlignment.h"
#include "tables-core.h"
#include "Lattice.h"
#include "SyntaxTree.h"

using namespace std;

RuleCollection::~RuleCollection()
{
	RemoveAllInColl(m_coll);
}

void RuleCollection::Add(const Global &global, Rule *rule, const SentenceAlignment &sentence)
{	
	Range spanS	= rule->GetSourceRange();
		
	// cartesian product of lhs
	Stack nontermNodes = sentence.GetLattice().GetNonTermNode(spanS);
	Stack::const_iterator iterStack;
	for (iterStack = nontermNodes.begin(); iterStack != nontermNodes.end(); ++iterStack)
	{
		const LatticeNode &node = **iterStack;
		assert(!node.IsTerminal());

		bool isValid;
		// create rules with LHS
		//cerr << "old:" << *rule << endl;
		Rule *newRule = new Rule(global, isValid, *rule, &node, sentence);
		
		if (!isValid)
		{ // lhs doesn't match non-term spans
			delete newRule;
			continue;
		}

		/*
		stringstream s;
		s << *newRule;
		if (s.str().find("Wiederaufnahme der [X] ||| resumption of the [X] ||| ||| 1") == 0)
		{
			cerr << "READY:" << *newRule << endl;
			g_debug = true;
		}
		else {
			g_debug = false;
		}
		*/
		
		typedef set<const Rule*, CompareRule>::iterator Iterator;
		pair<Iterator,bool> ret = m_coll.insert(newRule);
					
		if (ret.second)
		{
			//cerr << "ACCEPTED:" << *newRule << endl;
			//cerr << "";
		}
		else
		{
			//cerr << "REJECTED:" << *newRule << endl;
			delete newRule;
		}
		
	}
	
	delete rule;

}


std::ostream& operator<<(std::ostream &out, const RuleCollection &obj)
{	
	RuleCollection::CollType::const_iterator iter;
	for (iter = obj.m_coll.begin(); iter != obj.m_coll.end(); ++iter)
	{
		const Rule &rule = **iter;
		out << rule << endl;
	}
	
	return out;
}




