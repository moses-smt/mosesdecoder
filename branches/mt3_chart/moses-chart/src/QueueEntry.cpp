
#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOptionCollection.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

QueueEntry::QueueEntry(const TranslationOption &transOpt
											 , const std::map<WordsRange, ChartCell*> &allChartCells
											 , bool &isOK)
:m_transOpt(transOpt)
{
	isOK = false;
	const vector<WordsConsumed> &wordsConsumed = transOpt.GetChartRule().GetWordsConsumed();

	vector<WordsConsumed>::const_iterator iter;
	for (iter = wordsConsumed.begin(); iter != wordsConsumed.end(); ++iter)
	{
		const WordsConsumed &consumed = *iter;
		if (consumed.IsNonTerminal())
		{ // non-term
			const WordsRange &childRange = consumed.GetWordsRange();
			std::map<WordsRange, ChartCell*>::const_iterator iterCell = allChartCells.find(childRange);
			assert(iterCell != allChartCells.end());

			const ChartCell &childCell = *iterCell->second;

			if (childCell.GetSortedHypotheses().size() == 0)
			{ // can't create hypo out of this. child cell is empty
				return;
			}

			ChildEntry childEntry(0, childCell);
			m_childEntries.insert(m_childEntries.end(), childEntry);
		}
	}

	isOK = true;
}

QueueEntry::QueueEntry(const QueueEntry &copy, size_t childEntryIncr)
:m_transOpt(copy.m_transOpt)
,m_childEntries(copy.m_childEntries)
{
	ChildEntry &childEntry = m_childEntries[childEntryIncr];
	childEntry.IncrementPos();
}

void QueueEntry::CreateDeviants(ChartCell &currCell) const
{
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = m_childEntries[ind];

		if (childEntry.GetPos() + 1 < childEntry.GetChildCell().GetSize())
		{
			QueueEntry *newEntry = new QueueEntry(*this, ind);
			currCell.AddQueueEntry(newEntry);
		}
	}
}

}

