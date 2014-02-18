/*
 * Rules.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "Rules.h"
#include "Rule.h"
#include "Lattice.h"
#include "LatticeArc.h"

Rules::Rules(const Lattice &lattice, const AlignedSentence &alignedSentence)
:m_lattice(lattice)
,m_alignedSentence(alignedSentence)
{
	// fill collection with all 1-arc rules
	for (size_t i = 0; i < lattice.GetSize(); ++i) {
		const Lattice::LatticeNode &node = lattice.GetLatticeNode(i);
		for (size_t j = 0; j < node.size(); ++j) {
			const LatticeArc *arc = node[j];
			Rule *rule = new Rule(arc);
			m_activeRules.insert(rule);
		}
	}
}

Rules::~Rules() {
	// TODO Auto-generated destructor stub
}

void Rules::CreateRules()
{
	while (m_activeRules.size()) {
		std::set<Rule*> todoRules(m_activeRules);
		m_activeRules.clear();

		std::set<Rule*>::const_iterator iterRules;
		for (iterRules = todoRules.begin(); iterRules != todoRules.end(); ++iterRules) {
			Rule &rule = **iterRules;

			rule.Fillout();

			if (rule.IsValid()) {
				m_keepRules.insert(&rule);
			}

			Extend(rule);
		}
	}
}

void Rules::Extend(const Rule &rule)
{


}

