/*
 * RulePhrase.cpp
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#include <sstream>
#include "RulePhrase.h"
#include "RuleSymbol.h"

using namespace std;

extern bool g_debug;

int RulePhrase::Compare(const RulePhrase &other) const
{
  if (GetSize() != other.GetSize()) {
    return GetSize() < other.GetSize() ? -1 : +1;
  }

  for (size_t i = 0; i < m_coll.size(); ++i) {
    const RuleSymbol &symbol = *m_coll[i];
    const RuleSymbol &otherSymbol = *other.m_coll[i];
    int compare = symbol.Compare(otherSymbol);

    if (compare) {
      return compare;
    }
  }

  return 0;
}

void RulePhrase::Output(std::ostream &out) const
{
  for (size_t i =  0; i < m_coll.size(); ++i) {
    const RuleSymbol &symbol = *m_coll[i];
    symbol.Output(out);
    out << " ";
  }
}

std::string RulePhrase::Debug() const
{
  std::stringstream out;
  Output(out);
  return out.str();
}

