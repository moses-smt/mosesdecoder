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
	:m_startEnd(start, end)
	,m_label(label)
	{}

	virtual bool IsNonTerm() const
	{ return true; }
	virtual const std::string &GetString() const
	{ return m_label; }

	const ConsistentRange &GetOtherRange() const
	{ return *m_otherRange; }
	void SetOtherRange(const ConsistentRange &otherRange)
	{ m_otherRange = &otherRange; }

	int GetStart() const
	{ return m_startEnd.first; }

	int GetEnd() const
	{ return m_startEnd.second; }

	int GetWidth() const
	{ return m_startEnd.second - m_startEnd.first + 1; }

	int GetLowestAlignment() const;
	int GetHighestAlignment() const;

	void Output(std::ostream &out) const;
	void Debug(std::ostream &out) const;

  inline bool operator<(const ConsistentRange &other) const {
	return m_startEnd < other.m_startEnd;
  }

  bool Overlap(const ConsistentRange &other) const;
  bool Overlap(int pos) const;

protected:
	std::pair<int, int> m_startEnd;
	std::string m_label;
	const ConsistentRange *m_otherRange;
};

class ConsistentPhrase
{
public:
  ConsistentPhrase(int startSource, int endSource, int startTarget, int endTarget,
		  	  		const std::string &sourceLabel, const std::string &targetLabel);

  const ConsistentRange &GetConsistentRange(Moses::FactorDirection direction) const
  { return (direction == Moses::Input) ? m_ranges.first : m_ranges.second; }

  void Debug(std::ostream &out) const;

  inline bool operator<(const ConsistentPhrase &other) const {
	return m_ranges < other.m_ranges;
  }

protected:
  std::pair<ConsistentRange, ConsistentRange> m_ranges;
};

