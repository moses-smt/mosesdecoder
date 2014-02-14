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
,m_alignmentPos(copy.m_alignmentPos)
{
}


Rule::Rule(const LatticeNode *latticeNode)
:m_lhs(NULL)
{
	RuleElement element(*latticeNode);
	
	m_coll.push_back(element);
}

Rule::Rule(const Rule &prevRule, const LatticeNode *latticeNode)
:m_coll(prevRule.m_coll)
,m_lhs(NULL)
{	
	RuleElement element(*latticeNode);
	m_coll.push_back(element);
}

Rule::Rule(const Global &global, bool &isValid, const Rule &copy, const LatticeNode *lhs, const SentenceAlignment &sentence)
:m_coll(copy.m_coll)
,m_source(copy.m_source)
,m_target(copy.m_target)
,m_lhs(lhs)
{	
	CreateSymbols(global, isValid, sentence);
}

Rule::~Rule()
{
}

// helper for sort
struct CompareLatticeNodeTarget
{
 	bool operator() (const RuleElement *a, const RuleElement *b)
  {
		 const Range	 &rangeA = a->GetLatticeNode().GetTunnel().GetRange(1)
									,&rangeB = b->GetLatticeNode().GetTunnel().GetRange(1);
		 return rangeA.GetEndPos() < rangeB.GetEndPos();
	}
};

void Rule::CreateSymbols(const Global &global, bool &isValid, const SentenceAlignment &sentence)
{
	vector<RuleElement*> nonTerms;
		
	// source
	for (size_t ind = 0; ind < m_coll.size(); ++ind)
	{
		RuleElement &element = m_coll[ind];
		const LatticeNode &node = element.GetLatticeNode();
		if (node.IsTerminal())
		{
			size_t sourcePos = node.GetSourceRange().GetStartPos();
			const string &word = sentence.source[sourcePos];
			Symbol symbol(word, sourcePos);
			m_source.Add(symbol);			
		}
		else 
		{	// non-term
			const string &sourceWord = node.GetSyntaxNode(0).GetLabel();
			const string &targetWord = node.GetSyntaxNode(1).GetLabel();
			Symbol symbol(sourceWord, targetWord
										, node.GetTunnel().GetRange(0).GetStartPos(), node.GetTunnel().GetRange(0).GetEndPos()
										, node.GetTunnel().GetRange(1).GetStartPos(), node.GetTunnel().GetRange(1).GetEndPos()
										, node.GetSyntaxNode(0).IsSyntax(), node.GetSyntaxNode(1).IsSyntax());
			m_source.Add(symbol);		

			// store current pos within phrase
			element.m_alignmentPos.first = ind;

			// for target symbols
			nonTerms.push_back(&element);			
		}
		
	}
	
	// target
	isValid = true;
	
	const Range &lhsTargetRange = m_lhs->GetTunnel().GetRange(1);

	// check spans of target non-terms
	if (nonTerms.size())
	{
		// sort non-term rules elements by target range
		std::sort(nonTerms.begin(), nonTerms.end(), CompareLatticeNodeTarget());

		const Range &first = nonTerms.front()->GetLatticeNode().GetTunnel().GetRange(1);
		const Range &last = nonTerms.back()->GetLatticeNode().GetTunnel().GetRange(1);

		if (first.GetStartPos() < lhsTargetRange.GetStartPos()
				|| last.GetEndPos() > lhsTargetRange.GetEndPos())
		{			
			isValid = false;
		}
	}
	
	if (isValid)
	{
		size_t indNonTerm = 0;
		RuleElement *currNonTermElement = indNonTerm < nonTerms.size() ? nonTerms[indNonTerm] : NULL;
		for (size_t targetPos = lhsTargetRange.GetStartPos(); targetPos <= lhsTargetRange.GetEndPos(); ++targetPos)
		{		
			if (currNonTermElement && targetPos == currNonTermElement->GetLatticeNode().GetTunnel().GetRange(1).GetStartPos())
			{ // start of a non-term. print out non-terms & skip to the end
				
				const LatticeNode &node = currNonTermElement->GetLatticeNode();

				const string &sourceWord = node.GetSyntaxNode(0).GetLabel();
				const string &targetWord = node.GetSyntaxNode(1).GetLabel();
				Symbol symbol(sourceWord, targetWord
											, node.GetTunnel().GetRange(0).GetStartPos(), node.GetTunnel().GetRange(0).GetEndPos()
											, node.GetTunnel().GetRange(1).GetStartPos(), node.GetTunnel().GetRange(1).GetEndPos()
											, node.GetSyntaxNode(0).IsSyntax(), node.GetSyntaxNode(1).IsSyntax());
				m_target.Add(symbol);			
				
				// store current pos within phrase
				currNonTermElement->m_alignmentPos.second = m_target.GetSize() - 1;
				
				assert(currNonTermElement->m_alignmentPos.first != NOT_FOUND);

				targetPos = node.GetTunnel().GetRange(1).GetEndPos();
				indNonTerm++;
				currNonTermElement = indNonTerm < nonTerms.size() ? nonTerms[indNonTerm] : NULL;			
			}
			else 
			{ // term
				const string &word = sentence.target[targetPos];

				Symbol symbol(word, targetPos);
				m_target.Add(symbol);

			}
		}
				
		assert(indNonTerm == nonTerms.size());

		if (m_target.GetSize() > global.maxSymbolsTarget) {
		  isValid = false;
	    //cerr << "m_source=" << m_source.GetSize() << ":" << m_source << endl;
	    //cerr << "m_target=" << m_target.GetSize() << ":" << m_target << endl;
		}
	}	
}

bool Rule::MoreDefaultNonTermThanTerm() const
{
	size_t numTerm = 0, numDefaultNonTerm = 0;
	
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const RuleElement &element = *iter;
		const LatticeNode &node = element.GetLatticeNode();
		if (node.IsTerminal())
		{
			++numTerm;
		}
		else if (!node.IsSyntax())
		{
			++numDefaultNonTerm;
		}
	}
	
	bool ret = numDefaultNonTerm > numTerm;
	return ret;
}

bool Rule::SourceHasEdgeDefaultNonTerm() const
{
	assert(m_coll.size());
	const LatticeNode &first = m_coll.front().GetLatticeNode();
	const LatticeNode &last = m_coll.back().GetLatticeNode();

	// 1st
	if (!first.IsTerminal() && !first.IsSyntax())
	{
		return true;
	}
	if (!last.IsTerminal() && !last.IsSyntax())
	{
		return true;
	}
	
	return false;	
}

bool Rule::IsValid(const Global &global, const TunnelCollection &tunnelColl) const
{
	if (m_coll.size() == 1 && !m_coll[0].GetLatticeNode().IsTerminal()) {
		// can't be only 1 terminal
		return false;
	}

	/*
	if (MoreDefaultNonTermThanTerm()) {
	  // must have at least as many terms as non-syntax non-terms
	  return false;
	}
	*/

	if (!global.allowDefaultNonTermEdge && SourceHasEdgeDefaultNonTerm()) {
		return false;
	}
	
	if (GetNumSymbols() > global.maxSymbolsSource) {
		return false;
	}
	
	if (AdjacentDefaultNonTerms()) {
		return false;
	}
	
	if (!IsHole(tunnelColl)) {
		return false;
	}

	if (NonTermOverlap()) {
		return false;
	}
	if (!WithinNonTermSpans(global)) {
		return false;
	}

	/*
	std::pair<size_t, size_t> spanS	= GetSpan(0)
														,spanT= GetSpan(1);

	if (tunnelColl.NumUnalignedWord(0, spanS.first, spanS.second) >= global.maxUnaligned)
		return false;
	if (tunnelColl.NumUnalignedWord(1, spanT.first, spanT.second) >= global.maxUnaligned)
		return false;
	*/
	
	return true;
}

bool Rule::NonTermOverlap() const
{
	vector<Range> ranges;
	
	CollType::const_iterator iter;
	for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
	{
		const RuleElement &element = *iter;
		if (!element.GetLatticeNode().IsTerminal())
		{
			const Range &range = element.GetLatticeNode().GetTunnel().GetRange(1);
			ranges.push_back(range);
		}
	}
	
	vector<Range>::const_iterator outerIter;
	for (outerIter = ranges.begin(); outerIter != ranges.end(); ++outerIter)
	{
		const Range &outer = *outerIter;
		vector<Range>::const_iterator innerIter;
		for (innerIter = outerIter + 1; innerIter != ranges.end(); ++innerIter)
		{
			const Range &inner = *innerIter;
			if (outer.Overlap(inner))
				return true;
		}
	}
	
	return false;
}

Range Rule::GetSourceRange() const
{
	assert(m_coll.size());
	const Range &first = m_coll.front().GetLatticeNode().GetSourceRange();
	const Range &last = m_coll.back().GetLatticeNode().GetSourceRange();
	
	Range ret(first.GetStartPos(), last.GetEndPos());
	return ret;
}


bool Rule::IsHole(const TunnelCollection &tunnelColl) const
{
	const Range &spanS	= GetSourceRange();
	const TunnelList &tunnels = tunnelColl.GetTunnels(spanS.GetStartPos(), spanS.GetEndPos());

	bool ret = tunnels.size() > 0;
	return ret;
}


bool Rule::CanRecurse(const Global &global, const TunnelCollection &tunnelColl) const
{
	if (GetNumSymbols() >= global.maxSymbolsSource) {
		return false;
	}
	if (AdjacentDefaultNonTerms()) {
		return false;
	}
	/*
	if (MaxNonTerm(global)) {
		return false;
	}
	*/
	if (NonTermOverlap()) {
		return false;
	}
	if (!WithinNonTermSpans(global)) {
		return false;
	}
	
	const Range spanS	= GetSourceRange();

	if (tunnelColl.NumUnalignedWord(0, spanS.GetStartPos(), spanS.GetEndPos()) >= global.maxUnaligned)
		return false;
//	if (tunnelColl.NumUnalignedWord(1, spanT.first, spanT.second) >= global.maxUnaligned)
//		return false;
	
	
	return true;
}

bool Rule::WithinNonTermSpans(const Global &global) const
{
	assert(m_coll.size());
	const RuleElement &ruleElement = m_coll.back();
	const LatticeNode &latticeNode = ruleElement.GetLatticeNode();
	if (!latticeNode.IsTerminal()) {
		// non-term
		bool isSyntax = latticeNode.IsSyntax();
		size_t minSpan = isSyntax ? global.minHoleSpanSourceSyntax : global.minHoleSpanSourceDefault;
		size_t maxSpan = isSyntax ? global.maxHoleSpanSourceSyntax : global.maxHoleSpanSourceDefault;

		size_t width = latticeNode.GetSourceRange().GetWidth();

		if (width < minSpan || width > maxSpan) {
			return false;
		}
	}

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
		if (!node->IsTerminal()  )
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
		if (!prevNode->IsTerminal() && !node->IsTerminal() && !prevNode->IsSyntax() && !node->IsSyntax() )
		{
			return true;
		}
		prevNode = node;
	}
	
	return false;
}



size_t Rule::GetNumSymbols() const
{
	size_t ret = m_coll.size();	
	return ret;
}

void Rule::CreateRules(RuleCollection &rules
											 , const Lattice &lattice
											 , const SentenceAlignment &sentence
											 , const Global &global)
{
	//static int debug = 0;

	assert(m_coll.size() > 0);
	const LatticeNode *latticeNode = &m_coll.back().GetLatticeNode();
	size_t endPos = latticeNode->GetSourceRange().GetEndPos() + 1;
	
	const Stack &stack = lattice.GetStack(endPos);
	
	Stack::const_iterator iter;
	for (iter = stack.begin(); iter != stack.end(); ++iter)
	{
		/*
		++debug;
		cerr << debug << " ";
		if (debug == 54) {
			cerr << " ";
		}
		*/

		const LatticeNode *newLatticeNode = *iter;
		Rule *newRule = new Rule(*this, newLatticeNode);
		//newRule->DebugOutput();
		//cerr << endl;
		
		const TunnelCollection &tunnels = sentence.GetTunnelCollection();
		if (newRule->CanRecurse(global, tunnels))
		{ // may or maynot be valid, but can continue to build on this rule
			newRule->CreateRules(rules, lattice, sentence, global);
		}
		
		if (newRule->IsValid(global, tunnels))
		{ // add to rule collection
			rules.Add(global, newRule, sentence);
		}	
		else 
		{
			delete newRule;
		}

	}
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
	const string &thisSourceLabel		= m_lhs->GetSyntaxNode(0).GetLabel();
	const string &otherSourceLabel	= compare.m_lhs->GetSyntaxNode(0).GetLabel();
	if (thisSourceLabel != otherSourceLabel)
	{
		ret = (thisSourceLabel < otherSourceLabel) ? -1 : +1;
		return ret;
	}

	const string &thisTargetLabel		= m_lhs->GetSyntaxNode(1).GetLabel();
	const string &otherTargetLabel	= compare.m_lhs->GetSyntaxNode(1).GetLabel();
	if (thisTargetLabel != otherTargetLabel)
	{
		ret = (thisTargetLabel < otherTargetLabel) ? -1 : +1;
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
	cerr << "Nodes:";
	for (size_t i = 0; i < m_coll.size(); ++i) {
		const RuleElement &element = m_coll[i];
		cerr << element.GetLatticeNode() << " ";
	}

	//cerr << " Output: ";
	//Output(cerr);
}

void Rule::Output(std::ostream &out) const
{

  stringstream strmeS, strmeT;

  std::vector<Symbol>::const_iterator iterSymbol;
  for (iterSymbol = m_source.begin(); iterSymbol != m_source.end(); ++iterSymbol)
  {
    const Symbol &symbol = *iterSymbol;
    strmeS << symbol << " ";
  }

  for (iterSymbol = m_target.begin(); iterSymbol != m_target.end(); ++iterSymbol)
  {
    const Symbol &symbol = *iterSymbol;
    strmeT << symbol << " ";
  }

  // lhs
  if (m_lhs)
  {
    strmeS << m_lhs->GetSyntaxNode(0).GetLabel();
    strmeT << m_lhs->GetSyntaxNode(1).GetLabel();
  }

  out << strmeS.str() << " ||| " << strmeT.str() << " ||| ";

  // alignment
  Rule::CollType::const_iterator iter;
  for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
  {
    const RuleElement &element = *iter;
    const LatticeNode &node = element.GetLatticeNode();
    bool isTerminal = node.IsTerminal();

    if (!isTerminal)
    {
      out << element.m_alignmentPos.first << "-" << element.m_alignmentPos.second << " ";
    }
  }

  out << "||| 1";

}

void Rule::OutputInv(std::ostream &out) const
{
  stringstream strmeS, strmeT;

  std::vector<Symbol>::const_iterator iterSymbol;
  for (iterSymbol = m_source.begin(); iterSymbol != m_source.end(); ++iterSymbol)
  {
    const Symbol &symbol = *iterSymbol;
    strmeS << symbol << " ";
  }

  for (iterSymbol = m_target.begin(); iterSymbol != m_target.end(); ++iterSymbol)
  {
    const Symbol &symbol = *iterSymbol;
    strmeT << symbol << " ";
  }

  // lhs
  if (m_lhs)
  {
    strmeS << m_lhs->GetSyntaxNode(0).GetLabel();
    strmeT << m_lhs->GetSyntaxNode(1).GetLabel();
  }

  out << strmeT.str() << " ||| " << strmeS.str() << " ||| ";

  // alignment
  Rule::CollType::const_iterator iter;
  for (iter = m_coll.begin(); iter != m_coll.end(); ++iter)
  {
    const RuleElement &element = *iter;
    const LatticeNode &node = element.GetLatticeNode();
    bool isTerminal = node.IsTerminal();

    if (!isTerminal)
    {
      out << element.m_alignmentPos.second << "-" << element.m_alignmentPos.first << " ";
    }
  }

  out << "||| 1";

}


