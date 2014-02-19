/*
 * Rule.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include <limits>
#include "Rule.h"
#include "Parameter.h"
#include "LatticeArc.h"

using namespace std;

Rule::Rule(const LatticeArc &arc)
{
	m_arcs.push_back(&arc);
}

Rule::Rule(const Rule &prevRule, const LatticeArc &arc)
:m_arcs(prevRule.m_arcs)
{
	m_arcs.push_back(&arc);
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

bool Rule::IsValid(const Parameter &params) const
{

}

bool Rule::CanExtend(const Parameter &params) const
{

}

void Rule::Fillout(const ConsistentPhrases &consistentPhrases)
{
  int sourceStart = m_arcs.front()->GetStart();
  int sourceEnd = m_arcs.back()->GetEnd();

  int targetStart = numeric_limits<int>::max();
  int targetEnd = -1;

  for (size_t i = 0; i < m_arcs.size(); ++i) {
	  const LatticeArc &arc = *m_arcs[i];
	  if (arc.GetStart() < targetStart) {
		  targetStart = arc.GetStart();
	  }
	  if (arc.GetEnd() > targetEnd) {
		  targetEnd = arc.GetEnd();
	  }
  }


}


Rule *Rule::Extend(const LatticeArc &arc) const
{
	Rule *ret = new Rule(*this, arc);

	return ret;
}
