
#include <stdio.h>
#include "ChartManager.h"
#include "ChartCell.h"
#include "ChartHypothesis.h"
#include "ChartTrellisPath.h"
#include "ChartTrellisPathList.h"
#include "ChartTrellisPathCollection.h"
#include "../../moses/src/StaticData.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

Manager::Manager(InputType const& source)
:m_source(source)
,m_transOptColl(source)
{
	const StaticData &staticData = StaticData::Instance();
	staticData.InitializeBeforeSentenceProcessing(source);
}

Manager::~Manager()
{
	map<WordsRange, ChartCell*>::iterator iter;
	for (iter = m_hypoStackColl.begin(); iter != m_hypoStackColl.end(); ++iter)
		delete iter->second;

	StaticData::Instance().CleanUpAfterSentenceProcessing();
}

void Manager::ProcessSentence()
{
	TRACE_ERR("Translating: " << m_source << endl);

	const StaticData &staticData = StaticData::Instance();
	staticData.ResetSentenceStats(m_source);
	const vector <DecodeGraph*>
			&decodeGraphList = staticData.GetDecodeGraphList();

		// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_transOptColl.CreateTranslationOptions(decodeGraphList);

	TRACE_ERR(m_transOptColl.GetTranslationOptionList(WordsRange(1, 2)) << endl);
	TRACE_ERR("Decoding: " << endl);
	Hypothesis::ResetHypoCount();

	// MAIN LOOP
	size_t size = m_source.GetSize();
	for (size_t width = 1; width <= size; ++width)
	{
		for (size_t startPos = 0; startPos <= size-width; ++startPos)
		{
			size_t endPos = startPos + width - 1;
			WordsRange range(startPos, endPos);

			ChartCell *cell = new ChartCell(startPos, endPos);
			cell->ProcessSentence(m_transOptColl.GetTranslationOptionList(range)
														,m_hypoStackColl);
			cell->PruneToSize(cell->GetMaxHypoStackSize());
			cell->SortHypotheses();

			TRACE_ERR(range << " = " << cell->GetSize() << endl);

			m_hypoStackColl.Set(range, cell);
		}
	}

	cerr << "Num of hypo = " << Hypothesis::GetHypoCount() << endl;
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	size_t size = m_source.GetSize();

	if (size == 0) // empty source
		return NULL;
	else
	{
		WordsRange range(0, size-1);
		std::map<WordsRange, ChartCell*>::const_iterator iterCell = m_hypoStackColl.find(range);

		assert(iterCell != m_hypoStackColl.end());

		const ChartCell &childCell = *iterCell->second;
		const std::vector<const Hypothesis*> &sortedHypos = childCell.GetSortedHypotheses();

		return (sortedHypos.size() > 0) ? sortedHypos[0] : NULL;
	}
}

void Manager::CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct) const
{
	size_t size = m_source.GetSize();
	if (count == 0 || size == 0)
		return;

	WordsRange range(0, size-1);
	std::map<WordsRange, ChartCell*>::const_iterator iterCell = m_hypoStackColl.find(range);
	assert(iterCell != m_hypoStackColl.end());

	const ChartCell &lastCell = *iterCell->second;

	vector<const Hypothesis*> sortedPureHypo = lastCell.GetSortedHypotheses();
	if (sortedPureHypo.size() == 0)
		return;

	TrellisPathCollection contenders;

	set<Phrase> distinctHyps;

	// add all pure paths
	vector<const Hypothesis*>::const_iterator iterBestHypo;
	for (iterBestHypo = sortedPureHypo.begin() 
			; iterBestHypo != sortedPureHypo.end()
			; ++iterBestHypo)
	{
		const Hypothesis *hypo = *iterBestHypo;
		MosesChart::TrellisPath *purePath = new TrellisPath(hypo);
		contenders.Add(purePath);
	}

	// factor defines stopping point for distinct n-best list if too many candidates identical
	size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  if (nBestFactor < 1) nBestFactor = 1000; // 0 = unlimited

	// MAIN loop
	for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * nBestFactor) ; iteration++)
	{
		// get next best from list of contenders
		TrellisPath *path = contenders.pop();
		assert(path);

		// create deviations from current best
		path->CreateDeviantPaths(contenders);		

		if(onlyDistinct)
		{
			Phrase tgtPhrase = path->GetOutputPhrase();
			if (distinctHyps.insert(tgtPhrase).second) 
        ret.Add(path);
			else
				delete path;

			const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
			if (nBestFactor > 0)
				contenders.Prune(count * nBestFactor);
		}
		else 
    {
		  ret.Add(path);
			contenders.Prune(count);
    }
	}
}

void Manager::CalcDecoderStatistics() const
{
}

} // namespace

