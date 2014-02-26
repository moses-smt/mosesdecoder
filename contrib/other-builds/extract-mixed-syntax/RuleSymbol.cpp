/*
 * RuleSymbol.cpp
 *
 *  Created on: 21 Feb 2014
 *      Author: hieu
 */

#include "RuleSymbol.h"

RuleSymbol::RuleSymbol() {
	// TODO Auto-generated constructor stub

}

RuleSymbol::~RuleSymbol() {
	// TODO Auto-generated destructor stub
}

bool RuleSymbol::operator<(const RuleSymbol &other) const
{
	return GetString() < other.GetString();
}
