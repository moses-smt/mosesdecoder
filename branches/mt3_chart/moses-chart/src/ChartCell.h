
#pragma once

#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include "../../moses/src/Word.h"
#include "../../moses/src/WordsRange.h"
#include "ChartHypothesis.h"
#include "QueueEntry.h"
#include "Cube.h"

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;
class ChartCellCollection;

class HypothesisRecombinationOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		// assert in same cell
		const Moses::WordsRange &rangeA	= hypoA->GetCurrSourceRange()
													, &rangeB	= hypoB->GetCurrSourceRange();
		assert(rangeA == rangeB);

		int ret = Moses::Word::Compare(hypoA->GetTargetLHS(), hypoB->GetTargetLHS());
		if (ret != 0)
			return (ret < 0);
		
		ret = hypoA->LMContextCompare(*hypoB);
		if (ret != 0)
			return (ret < 0);

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
	friend std::ostream& operator<<(std::ostream&, const ChartCell&);
public:

protected:
	typedef std::set<Hypothesis*, HypothesisRecombinationOrderer> HCType;
	HCType m_hypos;

	std::map<Moses::Word, OrderHypos> m_hyposOrdered;
	std::vector<Moses::Word> m_headWords;
	const Hypothesis *m_bestHypo;

	Moses::WordsRange m_coverage;
	Cube m_queueUnique;

	float m_bestScore; /**< score of the best hypothesis in collection */
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
											,const ChartCellCollection &allChartCells);
	size_t GetSize() const
	{ return m_hypos.size(); } 
	size_t GetMaxHypoStackSize() const
	{
		return m_maxHypoStackSize;
	}

	float GetThreshold() const
	{ return m_bestScore + m_beamWidth; }

	const OrderHypos &GetSortedHypotheses(const Moses::Word &headWord) const;
	void AddQueueEntry(QueueEntry *queueEntry);
	bool AddHypothesis(Hypothesis *hypo);

	void SortHypotheses();
	void PruneToSize(size_t newSize);

	//! remove hypothesis pointed to by iterator but don't delete the object
	void Detach(const HCType::iterator &iter);
	/** destroy Hypothesis pointed to by iterator (object pool version) */
	void Remove(const HCType::iterator &iter);

	const Hypothesis *GetBestHypothesis() const
	{ return m_bestHypo; }

	bool HeadwordExists(const Moses::Word &headWord) const;
	const std::vector<Moses::Word> &GetHeadwords() const
	{ return m_headWords; }

	void CleanupArcList();

	//! transitive comparison used for adding objects into set
	inline bool operator<(const ChartCell &compare) const
	{
		return m_coverage < compare.m_coverage;
	}

};

}

