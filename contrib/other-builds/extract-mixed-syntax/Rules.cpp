/*
 * Rules.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include "Rules.h"
#include "ConsistentPhrases.h"
#include "AlignedSentence.h"
#include "Rule.h"

Rules::Rules(const AlignedSentence &alignedSentence)
{
	const ConsistentPhrases &cps = alignedSentence.GetConsistentPhrases();
	ConsistentPhrases::const_iterator iter;
	for (iter = cps.begin(); iter != cps.end(); ++iter) {
		const ConsistentPhrase &cp = *iter;
		Rule *rule = new Rule(cp);
		m_todoRules.insert(rule);
	}

}

Rules::~Rules() {
	// TODO Auto-generated destructor stub
}

void Rules::CreateRules()
{
	while (!m_todoRules.empty()) {
		Rule *origRule = *m_todoRules.begin();
		m_todoRules.erase(m_todoRules.begin());

		Extend(*origRule);

		if (origRule->IsValid()) {
			m_keepRules.insert(origRule);
		}
	}
}

void Rules::Extend(const Rule &rule)
{

}
