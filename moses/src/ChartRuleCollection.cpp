// $Id: ChartRuleCollection.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <algorithm>
#include "ChartRuleCollection.h"
#include "ChartRule.h"
#include "WordsRange.h"

using namespace std;
using namespace Moses;

namespace Moses
{
#ifdef USE_HYPO_POOL
	ObjectPool<ChartRuleCollection> ChartRuleCollection::s_objectPool("ChartRuleCollection", 3000);
#endif

ChartRuleCollection::ChartRuleCollection()
{
	m_collection.reserve(200);
	m_scoreThreshold = std::numeric_limits<float>::infinity();
}

ChartRuleCollection::~ChartRuleCollection()
{
	RemoveAllInColl(m_collection);
}


class ChartRuleOrderer
{
public:
	bool operator()(const ChartRule* itemA, const ChartRule* itemB) const
	{
		return itemA->GetTargetPhrase().GetFutureScore() > itemB->GetTargetPhrase().GetFutureScore();
	}
};

void ChartRuleCollection::Add(const TargetPhraseCollection &targetPhraseCollection
															, const WordConsumed &wordConsumed
															, bool adhereTableLimit
															, size_t ruleLimit)
{
	TargetPhraseCollection::const_iterator iter, iterEnd;
	iterEnd = (!adhereTableLimit || ruleLimit == 0 || targetPhraseCollection.GetSize() < ruleLimit)
								? targetPhraseCollection.end() : targetPhraseCollection.begin() + ruleLimit;

	for (iter = targetPhraseCollection.begin(); iter != iterEnd; ++iter)
	{
		const TargetPhrase &targetPhrase = **iter;
		float score = targetPhrase.GetFutureScore();

		if (m_collection.size() < ruleLimit)
		{ // not yet filled out quota. add everything
			m_collection.push_back(new ChartRule(targetPhrase, wordConsumed));
			m_scoreThreshold = (score < m_scoreThreshold) ? score : m_scoreThreshold;
		}
		else if (score > m_scoreThreshold)
		{ // full but not bursting. add if better than worst score
			m_collection.push_back(new ChartRule(targetPhrase, wordConsumed));
		}

		// prune if bursting
		if (m_collection.size() > ruleLimit * 2)
		{
			std::nth_element(m_collection.begin()
										, m_collection.begin() + ruleLimit
										, m_collection.end()
										, ChartRuleOrderer());
			// delete the bottom half
			for (size_t ind = ruleLimit; ind < m_collection.size(); ++ind)
			{
				// make the best score of bottom half the score threshold
				const TargetPhrase &targetPhrase = m_collection[ind]->GetTargetPhrase();
				float score = targetPhrase.GetFutureScore();
				m_scoreThreshold = (score > m_scoreThreshold) ? score : m_scoreThreshold;
				delete m_collection[ind];
			}
			m_collection.resize(ruleLimit);
		}

	}
}

void ChartRuleCollection::CreateChartRules(size_t ruleLimit)
{
	if (m_collection.size() > ruleLimit)
	{
		std::nth_element(m_collection.begin()
									, m_collection.begin() + ruleLimit
									, m_collection.end()
									, ChartRuleOrderer());

		// delete the bottom half
		for (size_t ind = ruleLimit; ind < m_collection.size(); ++ind)
		{
			delete m_collection[ind];
		}
		m_collection.resize(ruleLimit);
	}

	// finalise creation of chart rules
	for (size_t ind = 0; ind < m_collection.size(); ++ind)
	{
		ChartRule &rule = *m_collection[ind];
		rule.CreateNonTermIndex();
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

}

