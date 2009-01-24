
#pragma once

#include <set>
#include <queue>
#include <map>
#include <vector>
#include "../../moses/src/WordsRange.h"
#include "ChartHypothesis.h"
#include "QueueEntry.h"

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;

class ChartHypothesisRecombinationOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		// assert in same cell
		const Moses::WordsRange &rangeA	= hypoA->GetCurrSourceRange()
													, &rangeB	= hypoB->GetCurrSourceRange();
		assert(rangeA == rangeB);

		int ret = hypoA->LMContextCompare(*hypoB);
		if (ret != 0)
		{
			return (ret < 0);
		}

		return false;
	}
};

// order by descending score
class ChartHypothesisScoreOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		return hypoA->GetTotalScore() > hypoB->GetTotalScore();
	}
};

class ChartCell
{
protected:
	typedef std::set<Hypothesis*, ChartHypothesisRecombinationOrderer> HCType;
	HCType m_hypos;
	std::vector<const Hypothesis*> m_hyposOrdered;

	Moses::WordsRange m_coverage;
	std::set<QueueEntry*, QueueEntryOrderer> m_queueUnique;

	float m_bestScore; /**< score of the best hypothesis in collection */
	float m_worstScore; /**< score of the worse hypthesis in collection */
	float m_beamWidth; /**< minimum score due to threashold pruning */
	size_t m_maxHypoStackSize; /**< maximum number of hypothesis allowed in this stack */
	bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */

	void ExpandQueueEntry(const QueueEntry &queueEntry);
	void RemoveQueueEntry(QueueEntry *queueEntry);

	/** add hypothesis to stack. Prune if necessary.
 * Returns false if equiv hypo exists in collection, otherwise returns true
 */
	std::pair<HCType::iterator, bool> Add(Hypothesis *hypothesis);

public:
	ChartCell(size_t startPos, size_t endPos);
	~ChartCell();

	void ProcessSentence(const TranslationOptionList &transOptList
											,const std::map<Moses::WordsRange, ChartCell*> &allChartCells);
	size_t GetSize() const
	{ return m_hyposOrdered.size(); }
	size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}

	const std::vector<const Hypothesis*> &GetSortedHypotheses() const
	{ return m_hyposOrdered; }
	void AddQueueEntry(QueueEntry *queueEntry);
	bool AddHypothesis(Hypothesis *hypo);

	void SortHypotheses();
	void PruneToSize(size_t newSize);

	//! remove hypothesis pointed to by iterator but don't delete the object
	void Detach(const HCType::iterator &iter);
	/** destroy Hypothesis pointed to by iterator (object pool version) */
	void Remove(const HCType::iterator &iter);

	//! transitive comparison used for adding objects into set
	inline bool operator<(const ChartCell &compare) const
	{
		return m_coverage < compare.m_coverage;
	}

};

}

