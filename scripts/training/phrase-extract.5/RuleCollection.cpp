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

using namespace std;

RuleCollection::~RuleCollection()
{
	RemoveAllInColl(m_coll);
}

void RuleCollection::Add(Rule *rule, const SentenceAlignment &sentence)
{	
	std::pair<size_t, size_t> spanS	= rule->GetSpan(0)
														,spanT= rule->GetSpan(1);
	const SyntaxNodes nodesS = sentence.sourceTree.GetNodesForLHS(spanS.first, spanS.second);
	const SyntaxNodes nodesT = sentence.targetTree.GetNodesForLHS(spanT.first, spanT.second);
	
	rule->CreateSymbols();
	
	// cartesian product of lhs
	SyntaxNodes::const_iterator iterS, iterT;
	for (iterS = nodesS.begin(); iterS != nodesS.end(); ++iterS)
	{
		const SyntaxNode *syntaxNodeS = *iterS;
		
		for (iterT = nodesT.begin(); iterT != nodesT.end(); ++iterT)
		{
			const SyntaxNode *syntaxNodeT = *iterT;
			
			// create rules with LHS
			Rule *newRule = new Rule(*rule, syntaxNodeS, syntaxNodeT);
			
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
			
			typedef set<const Rule*, ComparRule>::iterator Iterator;
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




