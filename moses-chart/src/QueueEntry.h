
#pragma once

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <iostream>
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/Word.h"
#include "ChartHypothesis.h"

namespace Moses
{
	class WordConsumed;
};

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;
class ChartCell;
class ChartCellCollection;
class QueueEntry;

class ChildEntry
{
	friend std::ostream& operator<<(std::ostream&, const ChildEntry&);

protected:
	size_t m_pos;
	const OrderHypos &m_orderedHypos;
	const Moses::Word &m_headWord;

public:
	ChildEntry(size_t pos, const OrderHypos &orderedHypos, const Moses::Word &headWord)
		:m_pos(pos)
		,m_orderedHypos(orderedHypos)
		,m_headWord(headWord)
	{}
	size_t GetPos() const
	{ return m_pos; }
	const OrderHypos &GetOrderHypos() const
	{ return m_orderedHypos; }
	size_t IncrementPos()
	{ return m_pos++; }
	const Moses::Word &GetHeadWord() const
	{ return m_headWord; }

};


class QueueEntry
{
	friend std::ostream& operator<<(std::ostream&, const QueueEntry&);
protected:
	const TranslationOption &m_transOpt;
	std::vector<ChildEntry*> m_childEntries;

	float m_combinedScore;

	QueueEntry(const QueueEntry &copy, size_t childEntryIncr);
	bool CreateChildEntry(const Moses::WordConsumed *wordsConsumed, const ChartCellCollection &allChartCells);

	void CalcScore();

public:
	QueueEntry(const TranslationOption &transOpt
						, const ChartCellCollection &allChartCells
						, bool &isOK);
	~QueueEntry();

	const TranslationOption &GetTranslationOption() const
	{ return m_transOpt; }
	const std::vector<ChildEntry*> &GetChildEntries() const
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
