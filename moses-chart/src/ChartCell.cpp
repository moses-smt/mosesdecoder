
#include <algorithm>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartTranslationOption.h"
#include "ChartCellCollection.h"
#include "QueueEntry.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace Moses
{
	extern bool g_debug;
}

namespace MosesChart
{

ChartCell::ChartCell(size_t startPos, size_t endPos)
:m_coverage(startPos, endPos)
{
	const StaticData &staticData = StaticData::Instance();
	m_nBestIsEnabled = staticData.IsNBestEnabled();
}

const HypoList &ChartCell::GetSortedHypotheses(const Moses::Word &headWord) const
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator 
			iter = m_hypoColl.find(headWord);
	assert(iter != m_hypoColl.end());
	return iter->second.GetSortedHypotheses();
}

bool ChartCell::AddHypothesis(Hypothesis *hypo, bool prune)
{
	const Word &targetLHS = hypo->GetTargetLHS();
	return m_hypoColl[targetLHS].AddHypothesis(hypo, prune);
 }

void ChartCell::PruneToSize()
{
	std::map<Moses::Word, HypothesisCollection>::iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		HypothesisCollection &coll = iter->second;
		coll.PruneToSize();
	}
}

void ChartCell::ProcessSentence(const TranslationOptionList &transOptList
																, const ChartCellCollection &allChartCells)
{
	const StaticData &staticData = StaticData::Instance();

	// add all trans opt into queue. using only 1st child node.
	TranslationOptionList::const_iterator iterList;
	for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
	{
		const TranslationOption &transOpt = **iterList;

		bool isOK;
		QueueEntry *queueEntry = new QueueEntry(transOpt, allChartCells, isOK);

		if (isOK)
			AddQueueEntry(queueEntry);
		else
			delete queueEntry;
	}
	
	// pluck things out of queue and add to hypo collection
	const size_t popLimit = staticData.GetCubePruningPopLimit();

	for (size_t numPops = 0; numPops < popLimit && !m_queueUnique.IsEmpty(); ++numPops)
	{
		QueueEntry *queueEntry = m_queueUnique.Pop();
		
		queueEntry->GetTranslationOption().GetTotalScore();
		Hypothesis *hypo = new Hypothesis(*queueEntry);
		assert(hypo);
				
		hypo->CalcScore();
		
		AddHypothesis(hypo, true);

		ExpandQueueEntry(*queueEntry);
	}

	// empty queue
	while (!m_queueUnique.IsEmpty())
	{
		m_queueUnique.Pop();
		// deleted by cube
	}

}

void ChartCell::SortHypotheses()
{
	//const StaticData &staticData = StaticData::Instance();
	//const Word &defOutputNonTerm = staticData.GetOutputDefaultNonTerminal();
	//const Word &topOutputNonTerm = staticData.GetOutputTopNonTerminal();
	
	//vector<const Hypothesis*> allHypos;
	
	// sort each mini cells & fill up target lhs list
	std::map<Moses::Word, HypothesisCollection>::iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{	
		HypothesisCollection &coll = iter->second;
		coll.SortHypotheses();
		
	}		
}
	
const Hypothesis *ChartCell::GetBestHypothesis() const
{
	const Hypothesis *ret = NULL;
	float bestScore = -std::numeric_limits<float>::infinity();


	std::map<Moses::Word, HypothesisCollection>::const_iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		const HypoList &sortedList = iter->second.GetSortedHypotheses();
		//assert(sortedList.size() > 0);
		
		if (sortedList.size() > 0)
		{
			const Hypothesis *hypo = sortedList[0];
			if (hypo->GetTotalScore() > bestScore)
			{
				bestScore = hypo->GetTotalScore();
				ret = hypo;
			}
		}
	}
	
	return ret;
}

void ChartCell::AddQueueEntry(QueueEntry *queueEntry)
{
	bool inserted = m_queueUnique.Add(queueEntry);
}

void ChartCell::ExpandQueueEntry(const QueueEntry &queueEntry)
{
	queueEntry.CreateDeviants(*this);
}

void ChartCell::CleanupArcList()
{
	// only necessary if n-best calculations are enabled
	if (!m_nBestIsEnabled) return;

	std::map<Moses::Word, HypothesisCollection>::iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		HypothesisCollection &coll = iter->second;
		coll.CleanupArcList();
	}
}

void ChartCell::OutputSizes(std::ostream &out) const
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		const Moses::Word &targetLHS = iter->first;
		const HypothesisCollection &coll = iter->second;

		out << targetLHS << "=" << coll.GetSize() << ",";
	}	
}

size_t ChartCell::GetSize() const
{
	size_t ret = 0;
	std::map<Moses::Word, HypothesisCollection>::const_iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		const HypothesisCollection &coll = iter->second;
		
		ret += coll.GetSize();
	}	
	
	return ret;
}

void ChartCell::ProcessUnaryRules(const TranslationOptionList &transOptList
																	,const ChartCellCollection &allChartCells)
{
	assert(m_queueUnique.IsEmpty());
	
	// add all trans opt into queue. using only 1st child node.
	TranslationOptionList::const_iterator iterList;
	for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
	{
		const TranslationOption &transOpt = **iterList;
		
		bool isOK;
		QueueEntry *queueEntry = new QueueEntry(transOpt, allChartCells, isOK);
		
		if (isOK)
			AddQueueEntry(queueEntry);
		else
			delete queueEntry;
	}
	
	// no cube pruning. do everything
	while (!m_queueUnique.IsEmpty())
	{
		QueueEntry *queueEntry = m_queueUnique.Pop();
		
		queueEntry->GetTranslationOption().GetTotalScore();
		Hypothesis *hypo = new Hypothesis(*queueEntry);
		assert(hypo);
		
		hypo->CalcScore();
		
		AddHypothesis(hypo, false);
		
		ExpandQueueEntry(*queueEntry);
	}

	assert(m_queueUnique.IsEmpty());
}

void ChartCell::SortDefaultOnly()
{
	const StaticData &staticData = StaticData::Instance();
	const Word &defOutputNonTerm = staticData.GetOutputDefaultNonTerminal();
	HypothesisCollection &coll = m_hypoColl[defOutputNonTerm];
	coll.SortDefaultOnly();
}

const std::vector<const Moses::Word*> ChartCell::GetTargetLHSList() const
{
	std::vector<const Moses::Word*> ret;
	
	std::map<Moses::Word, HypothesisCollection>::const_iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		const Word &lhs = iter->first;
		ret.push_back(&lhs);
	}
	return ret;
}

	
std::ostream& operator<<(std::ostream &out, const ChartCell &cell)
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator iterOutside;
	for (iterOutside = cell.m_hypoColl.begin(); iterOutside != cell.m_hypoColl.end(); ++iterOutside)
	{
		const Moses::Word &targetLHS = iterOutside->first;
		const HypothesisCollection &coll = iterOutside->second;

		out << targetLHS << "="	
				<< coll << " ";
	}

	/*
	ChartCell::HCType::const_iterator iter;
	for (iter = cell.m_hypos.begin(); iter != cell.m_hypos.end(); ++iter)
	{
		const Hypothesis &hypo = **iter;
		out << hypo << endl;
	}
	 */
	
	return out;
}

} // namespace


