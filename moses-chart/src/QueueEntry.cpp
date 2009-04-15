
#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/ChartRule.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/WordConsumed.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

QueueEntry::QueueEntry(const TranslationOption &transOpt
											 , const ChartCellCollection &allChartCells
											 , bool &isOK)
:m_transOpt(transOpt)
{
	isOK = false;

	const WordConsumed *wordsConsumed = &transOpt.GetChartRule().GetLastWordConsumed();
	isOK = CreateChildEntry(wordsConsumed, allChartCells);

	if (isOK)
		CalcScore();
}

bool QueueEntry::CreateChildEntry(const Moses::WordConsumed *wordsConsumed, const ChartCellCollection &allChartCells)
{
	bool ret;
	// recursvile do the 1st first
	const WordConsumed *prevWordsConsumed = wordsConsumed->GetPrevWordsConsumed();
	if (prevWordsConsumed)
		ret = CreateChildEntry(prevWordsConsumed, allChartCells);
	else
		ret = true;

	if (ret && wordsConsumed->IsNonTerminal())
	{ // non-term
		const WordsRange &childRange = wordsConsumed->GetWordsRange();
		const ChartCell &childCell = allChartCells.Get(childRange);
		const Word &headWord = wordsConsumed->GetSourceWord();

		if (childCell.GetSortedHypotheses(headWord).size() == 0)
		{ // can't create hypo out of this. child cell is empty
			return false;
		}

		const Moses::Word &nonTerm = wordsConsumed->GetSourceWord();
		assert(nonTerm.IsNonTerminal());
		ChildEntry *childEntry = new ChildEntry(0, childCell.GetSortedHypotheses(nonTerm), nonTerm);
		m_childEntries.push_back(childEntry);
	}

	return ret;
}

QueueEntry::QueueEntry(const QueueEntry &copy, size_t childEntryIncr)
:m_transOpt(copy.m_transOpt)
{
	// deep copy of child entries
	//m_childEntries(copy.m_childEntries)
	std::vector<ChildEntry*>::const_iterator iter;
	for (iter = copy.m_childEntries.begin(); iter != copy.m_childEntries.end(); ++iter)
	{
		const ChildEntry &origEntry = **iter;
		ChildEntry *newEntry = new ChildEntry(origEntry);
		m_childEntries.push_back(newEntry);
	}

	ChildEntry &childEntry = *m_childEntries[childEntryIncr];
	childEntry.IncrementPos();

	CalcScore();
}

QueueEntry::~QueueEntry()
{
	Moses::RemoveAllInColl(m_childEntries);
}

void QueueEntry::CreateDeviants(ChartCell &currCell) const
{
	float threshold = currCell.GetThreshold();

	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = *m_childEntries[ind];

		if (childEntry.GetPos() + 1 < childEntry.GetOrderHypos().size())
		{
			QueueEntry *newEntry = new QueueEntry(*this, ind);
			if (newEntry->m_combinedScore > threshold)
			{
				currCell.AddQueueEntry(newEntry);
			}
			else
			{
				delete newEntry;
			}
		}
	}
}

void QueueEntry::CalcScore()
{
	m_combinedScore = m_transOpt.GetTotalScore();
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = *m_childEntries[ind];
		const Word &headWord = childEntry.GetHeadWord();

		const Hypothesis *hypo = childEntry.GetOrderHypos()[childEntry.GetPos()];
		m_combinedScore += hypo->GetTotalScore();
	}

}

}

