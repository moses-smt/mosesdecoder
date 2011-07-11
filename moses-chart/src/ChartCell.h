
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
	bool AddHypothesis(Hypothesis *hypo, bool prune);

	void SortHypotheses();
	void PruneToSize();
	void SortDefaultOnly();

	const Hypothesis *GetBestHypothesis() const;

	const std::vector<const Moses::Word*> GetTargetLHSList() const;

	void CleanupArcList();

	void OutputSizes(std::ostream &out) const;
	size_t GetSize() const;
	
	//! transitive comparison used for adding objects into set
	inline bool operator<(const ChartCell &compare) const
	{
		return m_coverage < compare.m_coverage;
	}
	
	void ProcessUnaryRules(const TranslationOptionList &transOptList
											 ,const ChartCellCollection &allChartCells);
	

};

}

