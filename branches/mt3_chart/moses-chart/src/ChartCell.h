
#pragma once

#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include "../../moses/src/Word.h"
#include "../../moses/src/WordsRange.h"
#include "ChartHypothesis.h"
#include "ChartHypothesisCollection.h"
#include "QueueEntry.h"
#include "Cube.h"

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;
class ChartCellCollection;

class ChartCell
{
	friend std::ostream& operator<<(std::ostream&, const ChartCell&);
public:

protected:
	std::map<Moses::Word, HypothesisCollection> m_hypoColl;	
	std::vector<Moses::Word> m_headWords;

	Moses::WordsRange m_coverage;
	Cube m_queueUnique;

	bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */

	void ExpandQueueEntry(const QueueEntry &queueEntry);

public:
	ChartCell(size_t startPos, size_t endPos);

	void ProcessSentence(const TranslationOptionList &transOptList
											,const ChartCellCollection &allChartCells);

	const HypoList &GetSortedHypotheses(const Moses::Word &headWord) const;
	void AddQueueEntry(QueueEntry *queueEntry);
	bool AddHypothesis(Hypothesis *hypo);

	void SortHypotheses();
	void PruneToSize();

	const Hypothesis *GetBestHypothesis() const;

	bool HeadwordExists(const Moses::Word &headWord) const;
	const std::vector<Moses::Word> &GetHeadwords() const
	{ return m_headWords; }

	void CleanupArcList();

	void OutputSizes(std::ostream &out) const;
	size_t GetSize() const;
	
	//! transitive comparison used for adding objects into set
	inline bool operator<(const ChartCell &compare) const
	{
		return m_coverage < compare.m_coverage;
	}

	void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;

};

}

