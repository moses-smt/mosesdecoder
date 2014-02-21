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

using namespace std;

Rules::Rules(const AlignedSentence &alignedSentence)
{
	const ConsistentPhrases &allCPS = alignedSentence.GetConsistentPhrases();

	size_t size = alignedSentence.GetPhrase(Moses::Input).size();
	for (size_t sourceStart = 0; sourceStart < size; ++sourceStart) {
		for (size_t sourceEnd = sourceStart; sourceEnd < size; ++sourceEnd) {
			const ConsistentPhrases::Coll &cps = allCPS.GetColl(sourceStart, sourceEnd);

			ConsistentPhrases::Coll::const_iterator iter;
			for (iter = cps.begin(); iter != cps.end(); ++iter) {
				const ConsistentPhrase &cp = *iter;
				Rule *rule = new Rule(cp, alignedSentence);
				m_todoRules.insert(rule);
			}

		}
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
	int sourceMin = rule.GetNextSourcePosForNonTerm();
	int sourceMax = rule.GetConsistentPhrase().corners[1];

	for (int sourceStart = sourceMin; sourceStart <= sourceMax; ++sourceStart) {
		for (int sourceEnd = sourceStart; sourceEnd <= sourceMax; ++sourceEnd) {


		}
	}
}

void Rules::Debug(std::ostream &out) const
{
	std::set<Rule*>::const_iterator iter;
	out << "m_todoRules:" << endl;
	for (iter = m_todoRules.begin(); iter != m_todoRules.end(); ++iter) {
		const Rule &rule = **iter;
		rule.Debug(out);
		cerr << endl;
	}

	out << "m_keepRules:" << endl;
	for (iter = m_keepRules.begin(); iter != m_keepRules.end(); ++iter) {
		const Rule &rule = **iter;
		rule.Debug(out);
		cerr << endl;
	}

}
