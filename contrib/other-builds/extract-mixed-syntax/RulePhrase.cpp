/*
 * RulePhrase.cpp
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#include "RulePhrase.h"
#include "RuleSymbol.h"


bool RulePhrase::operator<(const RulePhrase &other) const
{
  if (GetSize() != other.GetSize()) {
	return GetSize() < other.GetSize();
  }

  for (size_t i = 0; i < m_coll.size(); ++i) {
	  const RuleSymbol &symbol = *m_coll[i];
	  const RuleSymbol &otherSymbol = *other.m_coll[i];

	  if (symbol < otherSymbol) {
		  return true;
	  }
  }

  return false;
}
