
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
class QueueEntryOrderer;

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
	inline bool operator<(const ChildEntry &compare) const
	{
		if (m_childCell < compare.m_childCell)
			return true;
		return (m_childCell == compare.m_childCell) && (m_pos < compare.m_pos);
	}

};

class QueueEntry
{
protected:
	const TranslationOption &m_transOpt;
	std::vector<ChildEntry> m_childEntries;

	QueueEntry(const QueueEntry &copy, size_t childEntryIncr);
public:
	QueueEntry(const TranslationOption &transOpt
						, const std::map<Moses::WordsRange, ChartCell*> &allChartCells
						, bool &isOK);
	const TranslationOption &GetTranslationOption() const
	{ return m_transOpt; }
	const std::vector<ChildEntry> &GetChildEntries() const
	{ return m_childEntries; }
	void CreateDeviants(ChartCell &currCell) const;

};

class QueueEntryOrderer
{
public:
	bool operator()(const QueueEntry* entryA, const QueueEntry* entryB) const
	{
		const TranslationOption *transOptA = &entryA->GetTranslationOption()
																,*transOptB = &entryB->GetTranslationOption();
		if (transOptA < transOptB)
			return true;

		const std::vector<ChildEntry> &childEntriesA = entryA->GetChildEntries()
																,&childEntriesB = entryB->GetChildEntries();

		std::set<ChildEntry> setA, setB;
		std::copy(childEntriesA.begin(), childEntriesA.end(), std::inserter(setA, setA.end()));
		std::copy(childEntriesB.begin(), childEntriesB.end(), std::inserter(setB, setB.end()));


		return (transOptA == transOptB) && (setA < setB);
		//return true;
	}
};

}
