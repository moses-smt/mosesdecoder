// $Id$
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

#include <algorithm>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartTranslationOption.h"
#include "ChartCellCollection.h"
#include "Cube.h"
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

ChartCell::ChartCell(size_t startPos, size_t endPos, Manager &manager)
:m_coverage(startPos, endPos)
,m_manager(manager)
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

bool ChartCell::AddHypothesis(Hypothesis *hypo)
{
	const Word &targetLHS = hypo->GetTargetLHS();
	return m_hypoColl[targetLHS].AddHypothesis(hypo, m_manager);
}

void ChartCell::PruneToSize()
{
	std::map<Moses::Word, HypothesisCollection>::iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		HypothesisCollection &coll = iter->second;
		coll.PruneToSize(m_manager);
	}
}

void ChartCell::ProcessSentence(const TranslationOptionList &transOptList
																, const ChartCellCollection &allChartCells)
{
	const StaticData &staticData = StaticData::Instance();

	Cube cube;

	// add all trans opt into queue. using only 1st child node.
	TranslationOptionList::const_iterator iterList;
	for (iterList = transOptList.begin(); iterList != transOptList.end(); ++iterList)
	{
		const TranslationOption &transOpt = **iterList;

		bool isOK;
		QueueEntry *queueEntry = new QueueEntry(transOpt, allChartCells, isOK);

		if (isOK)
			cube.Add(queueEntry);
		else
			delete queueEntry;
	}
	
	// pluck things out of queue and add to hypo collection
	const size_t popLimit = staticData.GetCubePruningPopLimit();

	for (size_t numPops = 0; numPops < popLimit && !cube.IsEmpty(); ++numPops)
	{
		QueueEntry *queueEntry = cube.Pop();
		
		queueEntry->GetTranslationOption().GetTotalScore();
		Hypothesis *hypo = new Hypothesis(*queueEntry, m_manager);
		assert(hypo);
				
		hypo->CalcScore();
		
		AddHypothesis(hypo);

		// Expand queue entry
	    queueEntry->CreateDeviants(cube);
	}
}

void ChartCell::SortHypotheses()
{
	// sort each mini cells & fill up target lhs list
	assert(m_headWords.empty());
	m_headWords.reserve(m_hypoColl.size());
	std::map<Moses::Word, HypothesisCollection>::iterator iter;
	for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter)
	{
		m_headWords.push_back(iter->first);

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
		assert(sortedList.size() > 0);
		
		const Hypothesis *hypo = sortedList[0];
		if (hypo->GetTotalScore() > bestScore)
		{
			bestScore = hypo->GetTotalScore();
			ret = hypo;
		};
	}
	
	return ret;
}

bool ChartCell::HeadwordExists(const Moses::Word &headWord) const
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator iter;
	iter = m_hypoColl.find(headWord);
	return (iter != m_hypoColl.end());
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

		out << targetLHS << "=" << coll.GetSize() << " ";
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

void ChartCell::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator iterOutside;
	for (iterOutside = m_hypoColl.begin(); iterOutside != m_hypoColl.end(); ++iterOutside)
	{		
		const HypothesisCollection &coll = iterOutside->second;
		coll.GetSearchGraph(translationId, outputSearchGraphStream);
	}
	
}

std::ostream& operator<<(std::ostream &out, const ChartCell &cell)
{
	std::map<Moses::Word, HypothesisCollection>::const_iterator iterOutside;
	for (iterOutside = cell.m_hypoColl.begin(); iterOutside != cell.m_hypoColl.end(); ++iterOutside)
	{
		const Moses::Word &targetLHS = iterOutside->first;
		cerr << targetLHS << ":" << endl;
		
		const HypothesisCollection &coll = iterOutside->second;
		cerr << coll;
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


