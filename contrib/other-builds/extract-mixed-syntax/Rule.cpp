/*
 * Rule.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include "Rule.h"
#include "AlignedSentence.h"

Rule::Rule(const ConsistentPhrase &consistentPhrase, const AlignedSentence &alignedSentence)
:m_source(consistentPhrase.GetWidth(Moses::Input))
{
  int sourcePos = 0;
  for (int i = consistentPhrase.corners[0]; i <= consistentPhrase.corners[1]; ++i) {
	  m_source[sourcePos] = alignedSentence.GetPhrase(Moses::Input)[i];
	  ++sourcePos;
  }
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

