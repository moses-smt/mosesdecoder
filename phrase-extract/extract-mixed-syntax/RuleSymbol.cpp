/*
 * RuleSymbol.cpp
 *
 *  Created on: 21 Feb 2014
 *      Author: hieu
 */

#include "RuleSymbol.h"

using namespace std;

RuleSymbol::RuleSymbol()
{
  // TODO Auto-generated constructor stub

}

RuleSymbol::~RuleSymbol()
{
  // TODO Auto-generated destructor stub
}

int RuleSymbol::Compare(const RuleSymbol &other) const
{
  if (IsNonTerm() != other.IsNonTerm()) {
    return IsNonTerm() ? -1 : +1;
  }

  string str = GetString();
  string otherStr = other.GetString();

  if (str == otherStr) {
    return 0;
  } else {
    return  (str < otherStr) ? -1 : +1;
  }
}
