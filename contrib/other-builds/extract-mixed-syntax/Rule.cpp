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
,m_isValid(true)
,m_canRecurse(true)
{
	CreateSource();
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

void Rule::CreateSource()
{
  const ConsistentPhrase *cp = NULL;
  size_t nonTermInd = 0;
  if (nonTermInd < m_nonterms.size()) {
	  cp = m_nonterms[nonTermInd];
  }

  for (int sourcePos = m_consistentPhrase.corners[0];
		  sourcePos <= m_consistentPhrase.corners[1];
		  ++sourcePos) {

	  const RuleSymbol *ruleSymbol;
	  if (cp && cp->corners[0] <= sourcePos && sourcePos <= cp->corners[1]) {
		  // replace words with non-term
		  ruleSymbol = cp;
		  sourcePos = cp->corners[1];
		  if (m_nonterms.size()) {
			  cp = m_nonterms[nonTermInd];
		  }

		  // move to next non-term
		  ++nonTermInd;
		  if (nonTermInd < m_nonterms.size()) {
			  cp = m_nonterms[nonTermInd];
		  }
		  else {
			  cp = NULL;
		  }
	  }
	  else {
		  ruleSymbol = m_alignedSentence.GetPhrase(Moses::Input)[sourcePos];
	  }

	  m_source.push_back(ruleSymbol);
  }
}

void Rule::Debug(std::ostream &out) const
{
  // source
  for (size_t i =  0; i < m_source.size(); ++i) {
	  const RuleSymbol &symbol = *m_source[i];
	  symbol.Debug(out);
	  out << " ";
  }

  // target
  out << "||| ";
  for (size_t i =  0; i < m_target.size(); ++i) {
	  const RuleSymbol &symbol = *m_target[i];
	  symbol.Debug(out);
	  out << " ";
  }

  // overall range
  out << "||| ";
  m_consistentPhrase.Debug(out);

}
