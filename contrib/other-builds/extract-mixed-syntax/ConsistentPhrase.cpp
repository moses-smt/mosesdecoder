/*
 * ConsistentPhrase.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <cassert>
#include "ConsistentPhrase.h"

int ConsistentRange::GetLowestAlignment() const
{
	assert(m_otherRange);
	return m_otherRange->GetStart();
}

int ConsistentRange::GetHighestAlignment() const
{
	assert(m_otherRange);
	return m_otherRange->GetEnd();
}

bool ConsistentRange::Overlap(const ConsistentRange &other) const
{
    if ( other.m_startEnd.second < m_startEnd.first
    		|| other.m_startEnd.first > m_startEnd.second) {
    	return false;
    }

    return true;
}

bool ConsistentRange::Overlap(int pos) const
{
  return (m_startEnd.first <= pos && pos <= m_startEnd.second) ?
	  true : false;
}

void ConsistentRange::Output(std::ostream &out) const
{
	out << m_label;
}

void ConsistentRange::Debug(std::ostream &out) const
{
	out << m_label << "[" << GetStart() << "-" << GetEnd() << "]";
}

/////////////////////////////////////////////////////////////////////////////////

ConsistentPhrase::ConsistentPhrase(int startSource, int endSource,
					int startTarget, int endTarget,
		  	  		const std::string &sourceLabel, const std::string &targetLabel)
:m_ranges(ConsistentRange(startSource, endSource, sourceLabel),
		ConsistentRange(startTarget, endTarget, targetLabel))
{
	m_ranges.first.SetOtherRange(m_ranges.second);
	m_ranges.second.SetOtherRange(m_ranges.first);
}

