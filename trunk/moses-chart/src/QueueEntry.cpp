
#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionList.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "Cube.h"
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
		ChildEntry childEntry(0, childCell.GetSortedHypotheses(nonTerm), nonTerm);
		m_childEntries.push_back(childEntry);
	}

	return ret;
}

QueueEntry::QueueEntry(const QueueEntry &copy, size_t childEntryIncr)
:m_transOpt(copy.m_transOpt)
,m_childEntries(copy.m_childEntries)
{
	ChildEntry &childEntry = m_childEntries[childEntryIncr];
	childEntry.IncrementPos();
	CalcScore();
}

QueueEntry::~QueueEntry()
{
	//Moses::RemoveAllInColl(m_childEntries);
}

void QueueEntry::CreateDeviants(Cube &cube) const
{
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = m_childEntries[ind];

		if (childEntry.HasMoreHypo())
		{
			QueueEntry *newEntry = new QueueEntry(*this, ind);
			cube.Add(newEntry);
		}
	}
}

void QueueEntry::CalcScore()
{
	m_combinedScore = m_transOpt.GetTotalScore();
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = m_childEntries[ind];

		const Hypothesis *hypo = childEntry.GetHypothesis();
		m_combinedScore += hypo->GetTotalScore();
	}

}

bool QueueEntry::operator<(const QueueEntry &compare) const
{ 
	if (&m_transOpt != &compare.m_transOpt)
		return &m_transOpt < &compare.m_transOpt;
	
	bool ret = m_childEntries < compare.m_childEntries;
	return ret;
}

	
std::ostream& operator<<(std::ostream &out, const ChildEntry &entry)
{
	out << *entry.GetHypothesis();
	return out;
}

std::ostream& operator<<(std::ostream &out, const QueueEntry &entry)
{
	out << entry.GetTranslationOption() << endl;
	std::vector<ChildEntry>::const_iterator iter;
	for (iter = entry.GetChildEntries().begin(); iter != entry.GetChildEntries().end(); ++iter)
	{
		out << *iter << endl;
	}
	return out;
}

}

