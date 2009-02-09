
#include <vector>
#include "ChartHypothesis.h"
#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOption.h"
#include "../../moses/src/TargetPhrase.h"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/DummyScoreProducers.h"
#include "../../moses/src/LMList.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

unsigned int Hypothesis::s_HypothesesCreated = 0;

Hypothesis::Hypothesis(const QueueEntry &queueEntry)
:m_targetPhrase(queueEntry.GetTranslationOption().GetChartRule().GetTargetPhrase())
,m_wordsConsumedTargetOrder(queueEntry.GetTranslationOption().GetChartRule().GetWordsConsumedTargetOrder())
,m_id(++s_HypothesesCreated)
,m_currSourceWordsRange(queueEntry.GetTranslationOption().GetSourceWordsRange())
,m_contextPrefix(Output)
,m_contextSuffix(Output)
,m_arcList(NULL)
{
	assert(m_targetPhrase.GetSize() == m_wordsConsumedTargetOrder.size());

	const std::vector<ChildEntry> &childEntries = queueEntry.GetChildEntries();

	vector<ChildEntry>::const_iterator iter;
	for (iter = childEntries.begin(); iter != childEntries.end(); ++iter)
	{
		const ChildEntry &childEntry = *iter;
		size_t pos = childEntry.GetPos();
		const Hypothesis *prevHypo = childEntry.GetChildCell().GetSortedHypotheses()[pos];

		m_prevHypos.push_back(prevHypo);
	}

	GetPrefix(m_contextPrefix, 2);
	GetSuffix(m_contextSuffix, 2);
}

Hypothesis::~Hypothesis()
{
	if (m_arcList) 
	{
		ArcList::iterator iter;
		for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter)
		{
			FREEHYPO(*iter);
		}
		m_arcList->clear();

		delete m_arcList;
	}
}

void Hypothesis::CreateOutputPhrase(Phrase &outPhrase) const
{
	for (size_t pos = 0; pos < m_targetPhrase.GetSize(); ++pos)
	{
		const Word &word = m_targetPhrase.GetWord(pos);
		if (word.GetFactor(0)->IsNonTerminal())
		{ // non-term. fill out with prev hypo
			size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			prevHypo->CreateOutputPhrase(outPhrase);
		}
		else
		{
			outPhrase.AddWord(word);
		}
	}
}

Phrase Hypothesis::GetOutputPhrase() const
{
	Phrase outPhrase(Output);
	CreateOutputPhrase(outPhrase);
	return outPhrase;
}

size_t Hypothesis::GetPrefix(Phrase &ret, size_t size) const
{
	for (size_t pos = 0; pos < m_targetPhrase.GetSize(); ++pos)
	{
		const Word &word = m_targetPhrase.GetWord(pos);
		if (word.GetFactor(0)->IsNonTerminal())
		{
			size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			size = prevHypo->GetPrefix(ret, size);
		}
		else
		{
			ret.AddWord(m_targetPhrase.GetWord(pos));
			size--;
		}

		if (size==0)
			break;
	}

	return size;
}

size_t Hypothesis::GetSuffix(Phrase &ret, size_t size) const
{
	for (int pos = (int) m_targetPhrase.GetSize() - 1; pos >= 0 ; --pos)
	{
		const Word &word = m_targetPhrase.GetWord(pos);

		if (word.GetFactor(0)->IsNonTerminal())
		{
			size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			size = prevHypo->GetSuffix(ret, size);
		}
		else
		{
			ret.PrependWord(m_targetPhrase.GetWord(pos));
			size--;
		}

		if (size==0)
			break;
	}

	return size;
}

int Hypothesis::LMContextCompare(const Hypothesis &other) const
{
	// prefix
	int ret = GetPrefix().Compare(other.GetPrefix());
	if (ret != 0)
		return ret;

	// suffix
	ret = GetSuffix().Compare(other.GetSuffix());
	if (ret != 0)
		return ret;

	return 0;
}

void Hypothesis::CalcScore()
{
	const StaticData &staticData = StaticData::Instance();

	// total scores from prev hypos
	std::vector<const Hypothesis*>::iterator iter;
	for (iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter)
	{
		const Hypothesis &prevHypo = **iter;
		const ScoreComponentCollection &scoreBreakdown = prevHypo.GetScoreBreakDown();

		m_scoreBreakdown.PlusEquals(scoreBreakdown);
	}

	// translation models & word penalty
	const ScoreComponentCollection &scoreBreakdown = m_targetPhrase.GetScoreBreakdown();
	m_scoreBreakdown.PlusEquals(scoreBreakdown);

	float retFullScore, retNGramScore;
	CalcLMScore(retFullScore, retNGramScore);

	m_totalScore	= m_scoreBreakdown.GetWeightedScore();
}

void Hypothesis::CalcLMScore(float &retFullScore, float &retNGramScore)
{
	Phrase outPhrase = GetOutputPhrase();
	
	retFullScore = 0;
	retNGramScore = 0;
	StaticData::Instance().GetAllLM().CalcScore(outPhrase, retFullScore, retNGramScore, &m_scoreBreakdown, false);
}

void Hypothesis::AddArc(Hypothesis *loserHypo)
{
	if (!m_arcList) {
		if (loserHypo->m_arcList)  // we don't have an arcList, but loser does
		{
			this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
			loserHypo->m_arcList = 0;                // prevent a double deletion
		}
		else
			{ this->m_arcList = new ArcList(); }
	} else {
		if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
			size_t my_size = m_arcList->size();
			size_t add_size = loserHypo->m_arcList->size();
			this->m_arcList->resize(my_size + add_size, 0);
			std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(Hypothesis *));
			delete loserHypo->m_arcList;
			loserHypo->m_arcList = 0;
		} else { // loserHypo doesn't have any arcs
		  // DO NOTHING
		}
	}
	m_arcList->push_back(loserHypo);
}

TO_STRING_BODY(Hypothesis)

// friend
ostream& operator<<(ostream& out, const Hypothesis& hypo)
{
	Phrase outPhrase(Output);
	hypo.CreateOutputPhrase(outPhrase);

	// words bitmap
	out << hypo.GetId()
		<< " " << hypo.GetScoreBreakDown().GetWeightedScore()
			<< " " << hypo.m_currSourceWordsRange
			<< " " << hypo.m_targetPhrase
			<< " " << outPhrase;


	return out;
}

}

