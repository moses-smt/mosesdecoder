/*
 * ConsistentPhrase.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "ConsistentPhrase.h"

ConsistentPhrase::ConsistentPhrase(int startSource, int endSource,
					int startTarget, int endTarget,
		  	  		const std::string &sourceLabel, const std::string &targetLabel)
:m_ranges(ConsistentRange(startSource, endSource, sourceLabel),
		ConsistentRange(startTarget, endTarget, targetLabel))
{
	m_ranges.first.SetOtherRange(m_ranges.second);
	m_ranges.second.SetOtherRange(m_ranges.first);
}

