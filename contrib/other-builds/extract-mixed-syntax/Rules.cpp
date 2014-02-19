/*
 * Rules.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#include <cassert>
#include "Rules.h"
#include "Rule.h"
#include "Lattice.h"
#include "LatticeArc.h"
#include "Parameter.h"
#include "moses/Util.h"

Rules::Rules(const Lattice &lattice, const AlignedSentence &alignedSentence)
:m_lattice(lattice)
,m_alignedSentence(alignedSentence)
{
	// fill collection with all 1-arc rules
	for (size_t i = 0; i < lattice.GetSize(); ++i) {
		const Lattice::Node &node = lattice.GetNode(i);
		for (size_t j = 0; j < node.size(); ++j) {
			const LatticeArc &arc = *node[j];
			Rule *rule = new Rule(arc);
			m_activeRules.insert(rule);
		}
	}
}

Rules::~Rules() {
	assert(m_activeRules.empty());
	Moses::RemoveAllInColl(m_keepRules);
}

void Rules::CreateRules(const Parameter &params, const ConsistentPhrases &consistentPhrases)
{
	while (m_activeRules.size()) {
		std::set<Rule*> todoRules(m_activeRules);
		m_activeRules.clear();

		std::set<Rule*>::const_iterator iterRules;
		for (iterRules = todoRules.begin(); iterRules != todoRules.end(); ++iterRules) {
			Rule *rule = *iterRules;

			rule->Fillout(consistentPhrases, m_alignedSentence);
			Extend(*rule, params);

			if (rule->IsValid(params)) {
				m_keepRules.insert(rule);
			}
			else {
				delete rule;
			}

		}
	}
}

void Rules::Extend(const Rule &rule, const Parameter &params)
{
	if (!rule.CanExtend(params)) {
		return;
	}

	const LatticeArc &lastArc = rule.GetLastArc();
	int nextPos = lastArc.GetEnd() + 1;

	if (nextPos < m_lattice.GetSize()) {
		// not at the end yet
		const Lattice::Node &node = m_lattice.GetNode(nextPos);
		Lattice::Node::const_iterator iterNode;

		for (iterNode = node.begin(); iterNode != node.end(); ++iterNode) {
			const LatticeArc &arc = **iterNode;
			Rule *newRule = rule.Extend(arc);
			m_activeRules.insert(newRule);
		}
	}


}

