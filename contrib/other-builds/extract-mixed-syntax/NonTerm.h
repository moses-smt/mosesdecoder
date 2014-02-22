/*
 * NonTerm.h
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */
#pragma once
#include <string>
#include "RuleSymbol.h"

class ConsistentPhrase;

class NonTerm : public RuleSymbol
{
public:
	NonTerm(const ConsistentPhrase &consistentPhrase,
			const std::string &source,
			const std::string &target);
	virtual ~NonTerm();

	const ConsistentPhrase &GetConsistentPhrase()
	{ return m_consistentPhrase; }

	virtual std::string Debug() const;
	virtual void Output(std::ostream &out) const;

protected:
	const ConsistentPhrase &m_consistentPhrase;
	std::string m_source, m_target;
};

