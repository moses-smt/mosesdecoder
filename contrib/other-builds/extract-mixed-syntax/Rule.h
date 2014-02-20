/*
 * Rule.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

class ConsistentPhrase;

class Rule {
public:
	Rule(const ConsistentPhrase &consistentPhrase);
	virtual ~Rule();

	bool IsValid() const
	{ return m_isValid; }

	bool CanRecurse() const
	{ return m_canRecurse; }

protected:
	bool m_isValid, m_canRecurse;
};

