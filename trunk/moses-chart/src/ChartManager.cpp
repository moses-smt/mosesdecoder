// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

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

namespace Moses
{
	extern bool g_debug;
}

namespace MosesChart
{

Manager::Manager(InputType const& source)
:m_source(source)
,m_hypoStackColl(source, *this)
,m_transOptColl(source, StaticData::Instance().GetDecodeStepVL(source), m_hypoStackColl)
{
//	const StaticData &staticData = StaticData::Instance();
//	staticData.InitializeBeforeSentenceProcessing(source);
}

Manager::~Manager()
{
//	StaticData::Instance().CleanUpAfterSentenceProcessing();
}

void Manager::ProcessSentence()
{
	VERBOSE(1,"Translating: " << m_source << endl);

	ResetSentenceStats(m_source);

	VERBOSE(2,"Decoding: " << endl);
	//Hypothesis::ResetHypoCount();

	// MAIN LOOP
	size_t size = m_source.GetSize();
	for (size_t width = 1; width <= size; ++width)
	{
		for (size_t startPos = 0; startPos <= size-width; ++startPos)
		{
			size_t endPos = startPos + width - 1;
			WordsRange range(startPos, endPos);
			TRACE_ERR(" " << range << "=");
				
			// create trans opt
			m_transOptColl.CreateTranslationOptionsForRange(startPos, endPos);
			//if (g_debug)
			//	cerr << m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos));
			
			// decode			
			ChartCell &cell = m_hypoStackColl.Get(range);

			cell.ProcessSentence(m_transOptColl.GetTranslationOptionList(range)
														,m_hypoStackColl);
			cell.PruneToSize();
			cell.CleanupArcList();
			cell.SortHypotheses();
			
			cerr << cell.GetSize();
			//cerr << cell << endl;
			//cell.OutputSizes(cerr);
		}
	}

	IFVERBOSE(1) {
		cerr << "Num of hypo = " << Hypothesis::GetHypoCount() << " --- cells:" << endl;
		
		for (size_t startPos = 0; startPos < size; ++startPos)
		{
			cerr.width(3);
			cerr << startPos << " ";
		}
		cerr << endl;
		for (size_t width = 1; width <= size; width++)
		{
			for( size_t space = 0; space < width-1; space++ )
			{
				cerr << "  ";
			}
			for (size_t startPos = 0; startPos <= size-width; ++startPos)
			{
				WordsRange range(startPos, startPos+width-1);
				cerr.width(3);
				cerr << m_hypoStackColl.Get(range).GetSize() << " ";
			}
			cerr << endl;
		}
	}
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	size_t size = m_source.GetSize();

	if (size == 0) // empty source
		return NULL;
	else
	{
		WordsRange range(0, size-1);
		const ChartCell &lastCell = m_hypoStackColl.Get(range);
		return lastCell.GetBestHypothesis();
	}
}

void Manager::CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct) const
{
	size_t size = m_source.GetSize();
	if (count == 0 || size == 0)
		return;
	
	TrellisPathCollection contenders;
	set<Phrase> distinctHyps;

	// add all pure paths
	WordsRange range(0, size-1);
	const ChartCell &lastCell = m_hypoStackColl.Get(range);
	const Hypothesis *hypo = lastCell.GetBestHypothesis();
	
	if (hypo == NULL)
	{ // no hypothesis
		return;
	}

	MosesChart::TrellisPath *purePath = new TrellisPath(hypo);
	contenders.Add(purePath);

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

void Manager::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const
{
	size_t size = m_source.GetSize();
	for (size_t width = 1; width <= size; ++width)
	{
		for (size_t startPos = 0; startPos <= size-width; ++startPos)
		{
			size_t endPos = startPos + width - 1;
			WordsRange range(startPos, endPos);
			TRACE_ERR(" " << range << "=");
			
	
			const ChartCell &cell = m_hypoStackColl.Get(range);
			cell.GetSearchGraph(translationId, outputSearchGraphStream);
		}
	}
	
}
	
} // namespace

