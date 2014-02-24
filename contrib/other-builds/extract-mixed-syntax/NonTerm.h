/*
 * NonTerm.h
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */
#pragma once
#include <string>
#include "RuleSymbol.h"
#include "moses/TypeDef.h"

class ConsistentPhrase;

class NonTerm : public RuleSymbol
{
public:
	NonTerm(const ConsistentPhrase &consistentPhrase,
			const std::string &source,
			const std::string &target);
	virtual ~NonTerm();

	const ConsistentPhrase &GetConsistentPhrase() const
	{ return m_consistentPhrase; }

	virtual bool IsNonTerm() const
	{ return true; }

	virtual std::string Debug() const;
	virtual void Output(std::ostream &out) const;
  void Output(std::ostream &out, Moses::FactorDirection direction) const;

  const std::string &GetLabel(Moses::FactorDirection direction) const;

protected:
	const ConsistentPhrase &m_consistentPhrase;
	std::string m_source, m_target;
};

