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
:m_source(startSource, endSource, sourceLabel)
,m_target(startTarget, endTarget, targetLabel)
{
	m_source.SetOtherRange(m_target);
	m_target.SetOtherRange(m_source);
}

