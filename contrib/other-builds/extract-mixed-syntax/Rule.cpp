/*
 * Rule.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "Rule.h"

Rule::Rule(const LatticeArc &arc)
{
	m_arcs.push_back(&arc);
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

bool Rule::IsValid() const
{

}

bool Rule::CanExtend() const
{

}

void Rule::Fillout()
{

}


Rule *Rule::Extend(const LatticeArc &arc) const
{
	Rule *ret = new Rule(*this);
	ret->m_arcs.push_back(&arc);

	return ret;
}
