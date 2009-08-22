
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

namespace MosesChart
{

ChartCell::ChartCell(size_t startPos, size_t endPos)
:m_coverage(startPos, endPos)
{
	const StaticData &staticData = StaticData::Instance();

	m_beamWidth = staticData.GetBeamWidth();
	m_maxHypoStackSize = staticData.GetMaxHypoStackSize();
	m_nBestIsEnabled = staticData.IsNBestEnabled();
	m_bestScore = -std::numeric_limits<float>::infinity();
}

ChartCell::~ChartCell()
{
	HCType::iterator iter;
	for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter)
	{
		Hypothesis *hypo = *iter;
		Hypothesis::Delete(hypo);
	}
	//Moses::RemoveAllInColl(m_hypos);
}

const OrderHypos &ChartCell::GetSortedHypotheses(const Moses::Word &headWord) const
{
	std::map<Moses::Word, std::vector<const Hypothesis*> >::const_iterator 
			iter = m_hyposOrdered.find(headWord);
	assert(iter != m_hyposOrdered.end());
	return iter->second;
}

bool ChartCell::AddHypothesis(Hypothesis *hypo)
{
	if (hypo->GetTotalScore() < m_bestScore + m_beamWidth)
	{ // really bad score. don't bother adding hypo into collection
	  StaticData::Instance().GetSentenceStats().AddDiscarded();
	  VERBOSE(3,"discarded, too bad for stack" << std::endl);
		Hypothesis::Delete(hypo);
		return false;
	}

	// over threshold, try to add to collection
	std::pair<HCType::iterator, bool> addRet = Add(hypo);
	if (addRet.second)
	{ // nothing found. add to collection
		return true;
  }

	// equiv hypo exists, recombine with other hypo
	HCType::iterator &iterExisting = addRet.first;
	Hypothesis *hypoExisting = *iterExisting;
	assert(iterExisting != m_hypos.end());

	//StaticData::Instance().GetSentenceStats().AddRecombination(*hypo, **iterExisting);

	// found existing hypo with same target ending.
	// keep the best 1
	if (hypo->GetTotalScore() > hypoExisting->GetTotalScore())
	{ // incoming hypo is better than the one we have
		VERBOSE(3,"better than matching hyp " << hypoExisting->GetId() << ", recombining, ");
		if (m_nBestIsEnabled) {
			hypo->AddArc(hypoExisting);
			Detach(iterExisting);
		} else {
			Remove(iterExisting);
		}

		bool added = Add(hypo).second;
		if (!added)
		{
			iterExisting = m_hypos.find(hypo);
			TRACE_ERR("Offending hypo = " << **iterExisting << endl);
			abort();
		}
		return false;
	}
	else
	{ // already storing the best hypo. discard current hypo
	  VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
		if (m_nBestIsEnabled) {
			hypoExisting->AddArc(hypo);
		} else {
			Hypothesis::Delete(hypo);
		}
		return false;
	}
}

pair<ChartCell::HCType::iterator, bool> ChartCell::Add(Hypothesis *hypo)
{
	std::pair<HCType::iterator, bool> ret = m_hypos.insert(hypo);
	if (ret.second)
	{ // equiv hypo doesn't exists
		VERBOSE(3,"added hyp to stack");

		// Update best score, if this hypothesis is new best
		if (hypo->GetTotalScore() > m_bestScore)
		{
			VERBOSE(3,", best on stack");
			m_bestScore = hypo->GetTotalScore();
		}

	    // Prune only if stack is twice as big as needed (lazy pruning)
		VERBOSE(3,", now size " << m_hypos.size());
		if (m_hypos.size() > 2*m_maxHypoStackSize-1)
		{
			PruneToSize(m_maxHypoStackSize);
		}
		else {
		  VERBOSE(3,std::endl);
		}
	}

	return ret;
}


void ChartCell::PruneToSize(size_t newSize)
{
	if (GetSize() > newSize) // ok, if not over the limit
	{
		priority_queue<float> bestScores;

		// push all scores to a heap
		// (but never push scores below m_bestScore+m_beamWidth)
		HCType::iterator iter = m_hypos.begin();
		float score = 0;
		while (iter != m_hypos.end())
		{
			Hypothesis *hypo = *iter;
			score = hypo->GetTotalScore();
			if (score > m_bestScore+m_beamWidth)
			{
				bestScores.push(score);
			}
			++iter;
		}

		// pop the top newSize scores (and ignore them, these are the scores of hyps that will remain)
		//  ensure to never pop beyond heap size
		size_t minNewSizeHeapSize = newSize > bestScores.size() ? bestScores.size() : newSize;
		for (size_t i = 1 ; i < minNewSizeHeapSize ; i++)
			bestScores.pop();

		// and remember the threshold
		float scoreThreshold = bestScores.top();

		// delete all hypos under score threshold
		iter = m_hypos.begin();
		while (iter != m_hypos.end())
		{
			Hypothesis *hypo = *iter;
			float score = hypo->GetTotalScore();
			if (score < scoreThreshold)
			{
				HCType::iterator iterRemove = iter++;
				Remove(iterRemove);
				StaticData::Instance().GetSentenceStats().AddPruning();
			}
			else
			{
				++iter;
			}
		}
		VERBOSE(3,", pruned to size " << m_hypos.size() << endl);

		IFVERBOSE(3)
		{
			TRACE_ERR("stack now contains: ");
			for(iter = m_hypos.begin(); iter != m_hypos.end(); iter++)
			{
				Hypothesis *hypo = *iter;
				TRACE_ERR( hypo->GetId() << " (" << hypo->GetTotalScore() << ") ");
			}
			TRACE_ERR( endl);
		}

		// desperation pruning
		if (m_hypos.size() > newSize * 2)
		{
			std::vector<Hypothesis*> hyposOrdered;

			// sort hypos
			std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(hyposOrdered, hyposOrdered.end()));
			std::sort(hyposOrdered.begin(), hyposOrdered.end(), ChartHypothesisScoreOrderer());

			//keep only |size|. delete the rest
			std::vector<Hypothesis*>::iterator iter;
			for (iter = hyposOrdered.begin() + (newSize * 2); iter != hyposOrdered.end(); ++iter)
			{
				Hypothesis *hypo = *iter;
				HCType::iterator iterFindHypo = m_hypos.find(hypo);
				assert(iterFindHypo != m_hypos.end());
				Remove(iterFindHypo);
			}
		}
	}
}

/** Remove hypothesis pointed to by iterator but don't delete the object. */
void ChartCell::Detach(const HCType::iterator &iter)
{
	m_hypos.erase(iter);
}

void ChartCell::Remove(const HCType::iterator &iter)
{
	Hypothesis *h = *iter;

	/*
	stringstream strme("");
	strme << h->GetOutputPhrase();
	string toFind = "the goal of gene scientists is ";
	size_t pos = toFind.find(strme.str());

	if (pos == 0)
	{
		cerr << pos << " " << strme.str() << *h << endl;
		cerr << *this << endl;
	}
	*/

	Detach(iter);
	Hypothesis::Delete(h);
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
		
		AddHypothesis(hypo);

		ExpandQueueEntry(*queueEntry);
	}

	// empty queue
	while (!m_queueUnique.IsEmpty())
	{
		QueueEntry *queueEntry = m_queueUnique.Pop();
	}

}

void ChartCell::SortHypotheses()
{
	if (m_hypos.size() == 0)
	{
	}
	else
	{
		// done everything for this cell. 
		// sort
		// put into buckets according to headwords
		HCType::iterator iter;
		for (iter = m_hypos.begin(); iter != m_hypos.end(); ++iter)
		{
			const Hypothesis *hypo = *iter;
			const Word &headWord = hypo->GetTargetLHS();
			std::vector<const Hypothesis*> &vec = m_hyposOrdered[headWord];
			vec.push_back(hypo);
		}

		std::map<Moses::Word, OrderHypos>::iterator iterPerLHS;
		for (iterPerLHS = m_hyposOrdered.begin(); iterPerLHS != m_hyposOrdered.end(); ++iterPerLHS)
		{
			OrderHypos &orderHypos = iterPerLHS->second;

			std::sort(orderHypos.begin(), orderHypos.end(), ChartHypothesisScoreOrderer());
		}
		
		// create list of headwords
		std::map<Moses::Word, OrderHypos>::const_iterator iterMap;
		for (iterMap = m_hyposOrdered.begin(); iterMap != m_hyposOrdered.end(); ++iterMap)
		{
			m_headWords.push_back(iterMap->first);
		}
	}
}

const Hypothesis *ChartCell::GetBestHypothesis() const
{
	const Hypothesis *ret = NULL;
	float bestScore = -std::numeric_limits<float>::infinity();


	std::map<Moses::Word, OrderHypos>::const_iterator iter;
	for (iter = m_hyposOrdered.begin(); iter != m_hyposOrdered.end(); ++iter)
	{
		const Hypothesis *hypo = iter->second[0];
		if (hypo->GetTotalScore() > bestScore)
		{
			bestScore = hypo->GetTotalScore();
			ret = hypo;
		};
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

bool ChartCell::HeadwordExists(const Moses::Word &headWord) const
{
	std::map<Moses::Word, OrderHypos>::const_iterator iter;
	iter = m_hyposOrdered.find(headWord);
	return (iter != m_hyposOrdered.end());
}

void ChartCell::CleanupArcList()
{
	// only necessary if n-best calculations are enabled
	if (!m_nBestIsEnabled) return;

	HCType::iterator iter;
	for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter)
	{
		Hypothesis *mainHypo = *iter;
		mainHypo->CleanupArcList();
	}
}

std::ostream& operator<<(std::ostream &out, const ChartCell &cell)
{
	ChartCell::HCType::const_iterator iter;
	for (iter = cell.m_hypos.begin(); iter != cell.m_hypos.end(); ++iter)
	{
		const Hypothesis &hypo = **iter;
		out << hypo << endl;
	}

	return out;
}

} // namespace


