/*
 * Rules.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include <sstream>
#include "Rules.h"
#include "ConsistentPhrase.h"
#include "ConsistentPhrases.h"
#include "AlignedSentence.h"
#include "Rule.h"

using namespace std;

Rules::Rules(const AlignedSentence &alignedSentence)
:m_alignedSentence(alignedSentence)
{
	const ConsistentPhrases &allCPS = alignedSentence.GetConsistentPhrases();

	size_t size = alignedSentence.GetPhrase(Moses::Input).size();
	for (size_t sourceStart = 0; sourceStart < size; ++sourceStart) {
		for (size_t sourceEnd = sourceStart; sourceEnd < size; ++sourceEnd) {
			const ConsistentPhrases::Coll &cps = allCPS.GetColl(sourceStart, sourceEnd);

			ConsistentPhrases::Coll::const_iterator iter;
			for (iter = cps.begin(); iter != cps.end(); ++iter) {
				const ConsistentPhrase &cp = *iter;
				CreateRules(cp, alignedSentence);
			}
		}
	}
}

Rules::~Rules() {
	// TODO Auto-generated destructor stub
}

void Rules::CreateRules(const ConsistentPhrase &cp, const AlignedSentence &alignedSentence)
{
	const ConsistentPhrase::NonTerms &nonTerms = cp.GetNonTerms();
	for (size_t i = 0; i < nonTerms.size(); ++i) {
		const NonTerm &nonTerm = nonTerms[i];
		Rule *rule = new Rule(nonTerm, alignedSentence);

		if (rule->IsValid()) {
			m_keepRules.insert(rule);
		}
		if (rule->CanRecurse()) {
			m_todoRules.insert(rule);
		}
	}
}

void Rules::CreateRules(const Parameter &params)
{
	while (!m_todoRules.empty()) {
		Rule *origRule = *m_todoRules.begin();
		m_todoRules.erase(m_todoRules.begin());

		Extend(*origRule, params);
	}
}

void Rules::Extend(const Rule &rule, const Parameter &params)
{
	const ConsistentPhrases &allCPS = m_alignedSentence.GetConsistentPhrases();
	int sourceMin = rule.GetNextSourcePosForNonTerm();
	int sourceMax = rule.GetConsistentPhrase().corners[1];

	for (int sourceStart = sourceMin; sourceStart <= sourceMax; ++sourceStart) {
		for (int sourceEnd = sourceStart; sourceEnd <= sourceMax; ++sourceEnd) {
			if (sourceStart == rule.GetConsistentPhrase().corners[0] &&
				sourceMin == rule.GetConsistentPhrase().corners[1]) {
				// don't cover whole rule with 1 non-term
				continue;
			}

			const ConsistentPhrases::Coll &cps = allCPS.GetColl(sourceStart, sourceEnd);
			Extend(rule, cps, params);
		}
	}
}

void Rules::Extend(const Rule &rule, const ConsistentPhrases::Coll &cps, const Parameter &params)
{
	ConsistentPhrases::Coll::const_iterator iter;
	for (iter = cps.begin(); iter != cps.end(); ++iter) {
		const ConsistentPhrase &cp = *iter;
		Extend(rule, cp, params);
	}
}

void Rules::Extend(const Rule &rule, const ConsistentPhrase &cp, const Parameter &params)
{
	Rule *newRule = new Rule(rule, cp);
	newRule->Prevalidate(params);

	if (newRule->IsValid()) {
		m_keepRules.insert(newRule);
	}
	if (newRule->CanRecurse()) {
		m_todoRules.insert(newRule);
	}

	// recursively extend
	Extend(*newRule, params);
}

std::string Rules::Debug() const
{
	stringstream out;

	std::set<Rule*>::const_iterator iter;
	out << "m_todoRules:" << endl;
	for (iter = m_todoRules.begin(); iter != m_todoRules.end(); ++iter) {
		const Rule &rule = **iter;
		out << rule.Debug() << endl;
	}

	out << "m_keepRules:" << endl;
	for (iter = m_keepRules.begin(); iter != m_keepRules.end(); ++iter) {
		const Rule &rule = **iter;
		out << rule.Debug() << endl;
	}

	return out.str();
}

void Rules::Output(std::ostream &out) const
{
	std::set<Rule*>::const_iterator iter;
	for (iter = m_keepRules.begin(); iter != m_keepRules.end(); ++iter) {
		const Rule &rule = **iter;
		rule.Output(out);
		out << endl;
	}
}
