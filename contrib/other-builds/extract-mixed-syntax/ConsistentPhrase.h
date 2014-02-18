/*
 * ConsistentPhrase.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include "LatticeArc.h"
#include "moses/TypeDef.h"

class ConsistentRange : public LatticeArc
{
public:
	ConsistentRange(int start, int end, const std::string &label)
	:m_start(start)
	,m_end(end)
	,m_label(label)
	{}

	virtual bool IsNonTerm() const
	{ return true; }
	virtual const std::string &GetString() const
	{ return m_label; }

	void SetOtherRange(const ConsistentRange &otherRange)
	{ m_otherRange = &otherRange; }

	int GetStart() const
	{ return m_start; }

	int GetEnd() const
	{ return m_end; }

protected:
	int m_start, m_end;
	std::string m_label;
	const ConsistentRange *m_otherRange;
};

class ConsistentPhrase
{
public:
  ConsistentPhrase(int startSource, int endSource, int startTarget, int endTarget,
		  	  		const std::string &sourceLabel, const std::string &targetLabel);

  const ConsistentRange &GetConsistentRange(Moses::FactorDirection direction) const
  { return (direction == Moses::Input) ? m_source : m_target; }

protected:
  ConsistentRange m_source, m_target;
};

