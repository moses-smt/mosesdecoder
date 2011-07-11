/*
 *  Rule.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <algorithm>
#include <sstream>
#include "Rule.h"
#include "Global.h"
#include "LatticeNode.h"
#include "Lattice.h"
#include "SentenceAlignment.h"
#include "Tunnel.h"
#include "TunnelCollection.h"
#include "RuleCollection.h"

using namespace std;

RuleElement::RuleElement(const RuleElement &copy)
:m_latticeNode(copy.m_latticeNode)
{
	symbolPos[0] = copy.symbolPos[0];
	symbolPos[1] = copy.symbolPos[1];
}

RuleElement::RuleElement(const LatticeNode &latticeNode, size_t sourcePos)
: m_latticeNode(&latticeNode)
{
	symbolPos[0] = sourcePos;
	//symbolPos[1] = 6666;
}


Rule::Rule(const LatticeNode *latticeNode)
:m_lhsS(NULL)
,m_lhsT(NULL)
{
	RuleElement element(*latticeNode, 0);
	
	m_coll.push_back(element);
	SortTargetLHS();
}

Rule::Rule(const Rule &prevRule, const LatticeNode *latticeNode)
:m_coll(prevRule.m_coll)
,m_lhsS(NULL)
,m_lhsT(NULL)
{
	const RuleElement &lastElement = m_coll.back();
	size_t nextSourcePos = lastElement.symbolPos[0] + lastElement.GetLatticeNode().GetNumSymbols(0);
	
	RuleElement element(*latticeNode, nextSourcePos);
	m_coll.push_back(element);
	SortTargetLHS();	
}

Rule::Rule(const Rule &copy, const SyntaxNode *lhsS, const SyntaxNode *lhsT)
:m_coll(copy.m_coll)
,m_source(copy.m_source)
,m_target(copy.m_target)
,m_lhsS(lhsS)
,m_lhsT(lhsT)
{	
	
}

Rule::~Rule()
{
}

bool Rule::IsValid(const Global &global, const TunnelCollection &holeColl) const
{
	if (m_coll.size() == 1 && !m_coll[0].GetLatticeNode().m_isTerminal) // can't be only 1 terminal
	{
		return false;
	}
	
	if (GetNumSymbols() > global.maxSymbolsSource)
	{
		return false;
	}
	
	if (AdjacentDefaultNonTerms())
	{
		return false;
	}
	
	if (!IsHole(holeColl))
	{
		return false;
	}
	
	std::pair<size_t, size_t> spanS	= GetSpan(0)
														,spanT= GetSpan(1);

	if (holeColl.NumUnalignedWord(0, spanS.first, spanS.second) >= global.maxUnaligned)
		return false;
	if (holeColl.NumUnalignedWord(1, spanT.first, spanT.second) >= global.maxUnaligned)
		return false;
	
	return true;
}

bool Rule::IsHole(const TunnelCollection &holeColl) const
{
	std::pair<size_t, size_t> spanS	= GetSpan(0)
														,spanT= GetSpan(1);
	
	const TunnelList &holes = holeColl.GetHoles(spanS.first, spanS.second);
	TunnelList::const_iterator iter;
	for (iter = holes.begin(); iter != holes.end(); ++iter)
	{
		const Tunnel &hole = *iter;
		if (spanT.first == hole.GetStart(1) && spanT.second == hole.GetEnd(1) )
		{
			return true;
		}
	}
	
	return false;
}


bool Rule::CanRecurse(const Global &global, const TunnelCollection &holeColl) const
{
	if (GetNumSymbols() >= global.maxSymbolsSource)
		return false;
	if (AdjacentDefaultNonTerms())
		return false;
	if (MaxNonTerm(global))
		return false;
	
	std::pair<size_t, size_t> spanS	= GetSpan(0)
														,spanT= GetSpan(1);

	if (holeColl.NumUnalignedWord(0, spanS.first, spanS.second) >= global.maxUnaligned)
		return false;
	if (holeColl.NumUnalignedWord(1, spanT.first, spanT.second) >= global.maxUnaligned)
		return false;
	
	
	return true;
}

bool Rule::MaxNonTerm(const Global &global) const
{
	//cerr << *this << endl;
	size_t numNonTerm = 0, numNonTermDefault = 0;
	
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const LatticeNode *node = &(*iter).GetLatticeNode();
		if (!node->m_isTerminal  )
		{
			numNonTerm++;
			if (!node->IsSyntax())
			{
				numNonTermDefault++;
			}
			if (numNonTerm >= global.maxNonTerm || numNonTermDefault >= global.maxNonTermDefault)
				return true;
		}
	}
	
	return false;
}


bool Rule::AdjacentDefaultNonTerms() const
{
	assert(m_coll.size() > 0);
	
	const LatticeNode *prevNode = &m_coll.front().GetLatticeNode();
	CollType::const_iterator iter;
	for (iter = m_coll.begin() + 1; iter != m_coll.end(); ++iter)
	{
		const LatticeNode *node = &(*iter).GetLatticeNode();
		if (!prevNode->m_isTerminal && !node->m_isTerminal && !prevNode->IsSyntax() && !node->IsSyntax() )
		{
			return true;
		}
		prevNode = node;
	}
	
	return false;
}



size_t Rule::GetNumSymbols() const
{
	size_t ret = 0;
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const LatticeNode &node = (*iter).GetLatticeNode();
		ret += node.GetNumSymbols(0);
	}
	
	return ret;
}

void Rule::CreateRules(RuleCollection &rules
											 , const Lattice &lattice
											 , const SentenceAlignment &sentence
											 , const Global &global)
{
	assert(m_coll.size() > 0);
	const LatticeNode *latticeNode = &m_coll.back().GetLatticeNode();
	size_t endPos = latticeNode->m_hole.GetEnd(0) + 1;
	
	const Stack &stack = lattice.GetStack(endPos);
	
	Stack::const_iterator iter;
	for (iter = stack.begin(); iter != stack.end(); ++iter)
	{
		const LatticeNode *newLatticeNode = *iter;
		Rule *newRule = new Rule(*this, newLatticeNode);
		//cerr << *newRule << endl;
		
		if (newRule->CanRecurse(global, *sentence.holeCollection))
		{ // may or maynot be valid, but can continue to build on this rule
			newRule->CreateRules(rules, lattice, sentence, global);
		}
		
		if (newRule->IsValid(global, *sentence.holeCollection))
		{ // add to rule collection
			rules.Add(newRule, sentence);
		}	
		else 
		{
			delete newRule;
		}

	}
}

std::pair<size_t, size_t> Rule::GetSpan(size_t direction) const
{
	const LatticeNode *first, *last;
	if (direction == 0)
	{
		first	= &m_coll.front().GetLatticeNode();
		last	= &m_coll.back().GetLatticeNode();
	}
	else
	{
		first	= &m_targetSorted.front()->GetLatticeNode();
		last	= &m_targetSorted.back()->GetLatticeNode();
	}

	std::pair<size_t, size_t>	ret(first->m_hole.GetStart(direction), last->m_hole.GetEnd(direction));
	return ret;
}

// helper for sort
struct CompareLatticeNodeTarget
{
 	bool operator() (const RuleElement *a, const RuleElement *b)
  {
		const Tunnel &holeA = a->GetLatticeNode().m_hole
							,&holeB = b->GetLatticeNode().m_hole;
 		return holeA.GetStart(1) < holeB.GetStart(1);
 	}
};


void Rule::SortTargetLHS()
{
	assert(m_coll.size() > 0);
	assert(m_targetSorted.size() == 0);
	
	// copy to target vec. just has pointers to orig vec
	CollType::iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		RuleElement &element = *iter;
		m_targetSorted.push_back(&element);
	}
	assert(m_coll.size() == m_targetSorted.size());
	
	std::sort(m_targetSorted.begin(), m_targetSorted.end(), CompareLatticeNodeTarget());
		
	// create co-indexes on target side	
	m_targetSorted[0]->symbolPos[1] = 0;

	for (size_t ind = 1; ind < m_targetSorted.size(); ++ind)
	{
		RuleElement *element = m_targetSorted[ind];
		RuleElement *prevElement = m_targetSorted[ind - 1];
		
		size_t nextSourcePos = prevElement->symbolPos[1] + prevElement->GetLatticeNode().GetNumSymbols(1);		
		element->symbolPos[1] = nextSourcePos;
	}
}

void Rule::CreateSymbols()
{
	assert(m_coll.size() == m_targetSorted.size());
	assert(m_targetSorted.size() > 0);

	CreateSymbolsInterna();
	
	// free up lattice harvesting
	m_targetSorted.clear();
	
}

void Rule::CreateSymbolsInterna()
{
	assert(m_source.GetSize() == 0);
	assert(m_target.GetSize() == 0);

	// source
	Rule::CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const LatticeNode &node = (*iter).GetLatticeNode();
		node.CreateSymbols(0, m_source);
	}
	
	// target
	Rule::CollTypeTarget::const_iterator iterTarget;
	for (iterTarget = m_targetSorted.begin(); iterTarget != m_targetSorted.end(); ++iterTarget)
	{
		const LatticeNode &node = (*iterTarget)->GetLatticeNode();
		node.CreateSymbols(1, m_target);
	}	
}


std::ostream& operator<<(std::ostream &out, const Rule &obj)
{
	stringstream strmeS, strmeT;

	bool createSymbols = false;
	if(obj.m_source.GetSize() == 0)
	{ // have to create symbol vectors. Need to clear it afterwards
		assert(obj.m_target.GetSize() == 0);
		createSymbols = true;
		
		const_cast<Rule&>(obj).CreateSymbolsInterna();
	}

	std::vector<Symbol>::const_iterator iterSymbol;
	for (iterSymbol = obj.m_source.begin(); iterSymbol != obj.m_source.end(); ++iterSymbol)
	{
		const Symbol &symbol = *iterSymbol;
		strmeS << symbol << " ";
	}

	for (iterSymbol = obj.m_target.begin(); iterSymbol != obj.m_target.end(); ++iterSymbol)
	{
		const Symbol &symbol = *iterSymbol;
		strmeT << symbol << " ";
	}
	
	if (createSymbols)
	{ // clear out symbol vector
		const_cast<Rule&>(obj).m_source.Clear();
		const_cast<Rule&>(obj).m_target.Clear();
	}
	
	// lhs
	if (obj.m_lhsS == NULL)
	{
		assert(obj.m_lhsT == NULL);
		assert(false);
	}
	else 
	{
		strmeS << obj.m_lhsS->GetLabel();
		strmeT << obj.m_lhsT->GetLabel();
	}

	
	out << strmeS.str() << " ||| " << strmeT.str() << " ||| ";
	
	// alignment
	Rule::CollType::const_iterator iter;
	for (iter = obj.m_coll.begin(); iter != obj.m_coll.end(); ++iter)
	{
		const RuleElement &element = *iter;
		const LatticeNode &node = element.GetLatticeNode();
		bool isTerminal = node.m_isTerminal;
		
		if (!isTerminal)
		{
				out << element.symbolPos[0] << "-" << element.symbolPos[1] << " ";
		}
	}
	
	out << "||| 1";
	
	return out;
}

std::string Rule::ToString() const
{
	stringstream strme;
	strme << *this;
	return strme.str();
}


bool Rule::operator<(const Rule &compare) const
{	
	/*
	if (g_debug)
	{
		cerr << *this << endl << compare;
		cerr << endl;
	}
	*/
	
	bool ret = Compare(compare) < 0;
	
	/*
	if (g_debug)
	{
		cerr << *this << endl << compare << endl << ret << endl << endl;
	}
	*/
	
	return ret;
}

int Rule::Compare(const Rule &compare) const
{ 	
	//cerr << *this << endl << compare << endl;
	assert(m_coll.size() > 0);
	assert(m_targetSorted.size() == 0);
	assert(m_source.GetSize() > 0);
	assert(m_target.GetSize() > 0);

	int ret = 0;
	
	// compare each fragment
	ret = m_source.Compare(compare.m_source);
	if (ret != 0)
	{
		return ret;
	}

	ret = m_target.Compare(compare.m_target);
	if (ret != 0)
	{
		return ret;
	}
	
	// compare lhs
	if (m_lhsS->GetLabel() != compare.m_lhsS->GetLabel())
	{
		ret = (m_lhsS->GetLabel() < compare.m_lhsS->GetLabel()) ? -1 : +1;
		return ret;
	}

	if (m_lhsT->GetLabel() != compare.m_lhsT->GetLabel())
	{
		ret = (m_lhsT->GetLabel() < compare.m_lhsT->GetLabel()) ? -1 : +1;
		return ret;
	}
	
	assert(ret == 0);
	return ret;
}


const LatticeNode &Rule::GetLatticeNode(size_t ind) const
{
	assert(ind < m_coll.size());
	return m_coll[ind].GetLatticeNode();
}

void Rule::DebugOutput() const
{
	std::stringstream strme;			
	strme << *this;								
	cerr << strme.str();						
	
}

