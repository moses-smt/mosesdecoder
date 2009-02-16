
#pragma once

#include <vector>
#include <map>
#include <queue>
#include <set>
#include "../../moses/src/WordsRange.h"

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;
class ChartCell;
class QueueEntry;

class ChildEntry
{
protected:
	size_t m_pos;
	const ChartCell *m_childCell;

public:
	ChildEntry(size_t pos, const ChartCell &childCell)
		:m_pos(pos)
		,m_childCell(&childCell)
	{}
	size_t GetPos() const
	{ return m_pos; }
	size_t IncrementPos()
	{ return m_pos++; }
	const ChartCell &GetChildCell() const
	{ return *m_childCell; }
};

class QueueEntry
{
protected:
	const TranslationOption &m_transOpt;
	std::vector<ChildEntry> m_childEntries;

	float m_combinedScore;

	QueueEntry(const QueueEntry &copy, size_t childEntryIncr);

	void CalcScore();

public:
	QueueEntry(const TranslationOption &transOpt
						, const std::map<Moses::WordsRange, ChartCell*> &allChartCells
						, bool &isOK);
	const TranslationOption &GetTranslationOption() const
	{ return m_transOpt; }
	const std::vector<ChildEntry> &GetChildEntries() const
	{ return m_childEntries; }
	float GetCombinedScore() const
	{ return m_combinedScore; }

	void CreateDeviants(ChartCell &currCell) const;

};

class QueueEntryOrderer
{
public:
	bool operator()(const QueueEntry* entryA, const QueueEntry* entryB) const
	{
		return (entryA->GetCombinedScore() > entryB->GetCombinedScore());
	}
};

}
