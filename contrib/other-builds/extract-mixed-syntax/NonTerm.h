/*
 * NonTerm.h
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */
#pragma once
#include "RuleSymbol.h"

class ConsistentPhrase;

class NonTerm : public RuleSymbol
{
public:
	NonTerm(const ConsistentPhrase &consistentPhrase);
	virtual ~NonTerm();

	const ConsistentPhrase &GetConsistentPhrase()
	{ return m_consistentPhrase; }

protected:
	const ConsistentPhrase &m_consistentPhrase;
};

