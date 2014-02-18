/*
 * ConsistentPhrase.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include "LatticeNode.h"

class ConsistentRange : public LatticeNode
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

protected:
	int m_start, m_end;
	std::string m_label;
	const ConsistentRange *m_otherRange;
};

class ConsistentPhrase
{
public:
  int startSource, endSource, startTarget, endTarget;

  ConsistentPhrase(int startSource, int endSource, int startTarget, int endTarget,
		  	  		const std::string &sourceLabel, const std::string &targetLabel);

protected:
  ConsistentRange m_source, m_target;
};

