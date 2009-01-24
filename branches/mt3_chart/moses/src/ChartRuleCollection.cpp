
#include "ChartRuleCollection.h"
#include "ChartRule.h"
#include "WordsRange.h"

using namespace std;
using namespace Moses;

namespace Moses
{

ChartRuleCollection::~ChartRuleCollection()
{
	Moses::RemoveAllInColl(m_collection);
}

void ChartRuleCollection::Add(const TargetPhraseCollection &targetPhraseCollection
															, const std::vector<WordsConsumed> &wordsConsumed
															, bool adhereTableLimit
															, size_t tableLimit)
{
	pair<std::set<std::vector<WordsConsumed> >::const_iterator, bool> pair
	 = m_wordsConsumed.insert(wordsConsumed);
	const std::vector<WordsConsumed> &wordsConsumedInserted = *pair.first;

	TargetPhraseCollection::const_iterator iter, iterEnd;
	iterEnd = (!adhereTableLimit || tableLimit == 0 || targetPhraseCollection.GetSize() < tableLimit) 
								? targetPhraseCollection.end() : targetPhraseCollection.begin() + tableLimit;

	for (iter = targetPhraseCollection.begin(); iter != iterEnd; ++iter)
	{
		const TargetPhrase &targetPhrase = **iter;
		ChartRule *rule = new ChartRule (targetPhrase, wordsConsumedInserted);
		m_collection.push_back(rule);
	}
}

std::ostream& operator<<(std::ostream &out, const ChartRuleCollection &coll)
{
	ChartRuleCollection::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const ChartRule &rule = **iter;
		out << rule << endl;
	}
	return out;
}

std::ostream& operator<<(std::ostream &out, const ChartRule &rule)
{
	out << rule.m_targetPhrase << endl;

	vector<WordsConsumed>::const_iterator iter;
	for (iter = rule.m_wordsConsumed.begin(); iter != rule.m_wordsConsumed.end(); ++iter)
	{
		out << iter->GetWordsRange() << " ";
	}

	return out;
}

}

