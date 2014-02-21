/*
 * Rule.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include "Rule.h"
#include "AlignedSentence.h"
#include "ConsistentPhrase.h"

Rule::Rule(const ConsistentPhrase &consistentPhrase, const AlignedSentence &alignedSentence)
:m_consistentPhrase(consistentPhrase)
,m_alignedSentence(alignedSentence)
{
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

void Rule::CreateSource()
{
  size_t nonTermInd = 0;

  const ConsistentPhrase *cp = NULL;
  if (m_nonterms.size()) {
	  cp = m_nonterms[nonTermInd];
  }

  for (int sourcePos = m_consistentPhrase.corners[0];
		  sourcePos <= m_consistentPhrase.corners[1];
		  ++sourcePos) {

	  if (cp && cp->corners[0] <= sourcePos && sourcePos <= cp->corners[1]) {

	  }

	  Word *word = m_alignedSentence.GetPhrase(Moses::Input)[sourcePos];

	  m_source.push_back(word);
  }
}
