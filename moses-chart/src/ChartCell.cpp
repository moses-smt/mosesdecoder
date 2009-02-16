
#include <algorithm>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartTranslationOption.h"
#include "QueueEntry.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

/*
class QueueEntryOrderer
{
public:
	bool operator()(const QueueEntry* hypoA, const QueueEntry* hypoB) const
	{

		return true;
	}
};
*/

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
	Moses::RemoveAllInColl(m_hypos);
}

bool ChartCell::AddHypothesis(Hypothesis *hypo)
{
	if (hypo->GetTotalScore() < m_bestScore + m_beamWidth)
	{ // really bad score. don't bother adding hypo into collection
	  StaticData::Instance().GetSentenceStats().AddDiscarded();
	  VERBOSE(3,"discarded, too bad for stack" << std::endl);
		FREEHYPO(hypo);
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
			FREEHYPO(hypo);
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
	if (m_hypos.size() > newSize) // ok, if not over the limit
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
		VERBOSE(3,", pruned to size " << GetSize() << endl);

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
	delete h;
}

void ChartCell::ProcessSentence(const TranslationOptionList &transOptList
																, const std::map<WordsRange, ChartCell*> &allChartCells)
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

	for (size_t numPops = 0; numPops < popLimit && !m_queueUnique.empty(); ++numPops)
	{
		QueueEntry *queueEntry = *m_queueUnique.begin();

		Hypothesis *hypo = new Hypothesis(*queueEntry);
		hypo->CalcScore();

		AddHypothesis(hypo);

		ExpandQueueEntry(*queueEntry);
		RemoveQueueEntry(queueEntry);
	}

	// empty queue
	while (!m_queueUnique.empty())
	{
		QueueEntry *queueEntry = *m_queueUnique.begin();
		RemoveQueueEntry(queueEntry);
	}

}

void ChartCell::SortHypotheses()
{
	// done everything for this cell. order hypos
	std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(m_hyposOrdered, m_hyposOrdered.end()));
	std::sort(m_hyposOrdered.begin(), m_hyposOrdered.end(), ChartHypothesisScoreOrderer());
}

void ChartCell::RemoveQueueEntry(QueueEntry *queueEntry)
{
	bool erased = m_queueUnique.erase(queueEntry);
	assert(erased);
	delete queueEntry;
}

void ChartCell::AddQueueEntry(QueueEntry *queueEntry)
{
	std::pair<set<QueueEntry*, QueueEntryOrderer>::iterator, bool> inserted = m_queueUnique.insert(queueEntry);

	if (inserted.second)
	{ // inserted ok. doing nothing
	}
	else
	{ // already exists. delete
		delete queueEntry;
	}
}

void ChartCell::ExpandQueueEntry(const QueueEntry &queueEntry)
{
	queueEntry.CreateDeviants(*this);
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


