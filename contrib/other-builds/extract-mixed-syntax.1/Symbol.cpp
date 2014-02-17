/*
 *  Symbol.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 21/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <cassert>
#include "Symbol.h"

using namespace std;

Symbol::Symbol(const std::string &label, size_t pos)
:m_label(label)
,m_isTerminal(true)
,m_span(2)
{
	m_span[0].first = pos;
}

Symbol::Symbol(const std::string &labelS, const std::string &labelT
							 , size_t startS, size_t endS
							 , size_t startT, size_t endT
							 , bool isSourceSyntax, bool isTargetSyntax)
:m_label(labelS)
,m_labelT(labelT)
,m_isTerminal(false)
,m_span(2)
,m_isSourceSyntax(isSourceSyntax)
,m_isTargetSyntax(isTargetSyntax)
{
	m_span[0] = std::pair<size_t, size_t>(startS, endS);
	m_span[1] = std::pair<size_t, size_t>(startT, endT);
}

int CompareNonTerm(bool thisIsSyntax, bool otherIsSyntax
									 , const std::pair<size_t, size_t> &thisSpan, const std::pair<size_t, size_t> &otherSpan
									 , std::string thisLabel, std::string otherLabel)
{
	if (thisIsSyntax != otherIsSyntax)
	{ // 1 is [X] & the other is [NP] on the source
		return thisIsSyntax ? -1 : +1;
	}

	assert(thisIsSyntax == otherIsSyntax);
	if (thisIsSyntax)
	{ // compare span & label
		if (thisSpan != otherSpan)
			return thisSpan < otherSpan ? -1 : +1;
		if (thisLabel != otherLabel)
			return thisLabel < otherLabel ? -1 : +1;
	}
	
	return 0;
}

int Symbol::Compare(const Symbol &other) const
{
	if (m_isTerminal != other.m_isTerminal)
		return m_isTerminal ? -1 : +1;
	
	assert(m_isTerminal == other.m_isTerminal);
	if (m_isTerminal)
	{ // compare labels & pos
		if (m_span[0].first != other.m_span[0].first)
			return (m_span[0].first < other.m_span[0].first) ? -1 : +1;
		
		if (m_label != other.m_label)
			return (m_label < other.m_label) ? -1 : +1;
		
	}
	else 
	{ // non terms
		int ret = CompareNonTerm(m_isSourceSyntax, other.m_isSourceSyntax
														,m_span[0], other.m_span[0]
														 ,m_label, other.m_label);
		if (ret != 0)
			return ret;
			
		ret = CompareNonTerm(m_isTargetSyntax, other.m_isTargetSyntax
												 ,m_span[1], other.m_span[1]
												 ,m_label, other.m_label);
		if (ret != 0)
			return ret;
	}
	
	return 0;
}


std::ostream& operator<<(std::ostream &out, const Symbol &obj)
{
	if (obj.m_isTerminal)
		out << obj.m_label;
	else 
		out << obj.m_label + obj.m_labelT;

	return out;
}

