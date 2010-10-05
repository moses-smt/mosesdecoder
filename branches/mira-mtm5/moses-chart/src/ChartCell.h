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

namespace Moses
{
class ChartTranslationOptionList;	
}

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class TranslationOption;
class ChartCellCollection;
class Manager;

class ChartCell
{
	friend std::ostream& operator<<(std::ostream&, const ChartCell&);
public:

protected:
	std::map<Moses::Word, HypothesisCollection> m_hypoColl;	
	std::vector<Moses::Word> m_headWords;

	Moses::WordsRange m_coverage;

	bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */
	Manager &m_manager;
	
public:
	ChartCell(size_t startPos, size_t endPos, Manager &manager);

	void ProcessSentence(const Moses::ChartTranslationOptionList &transOptList
											,const ChartCellCollection &allChartCells);

	const HypoList &GetSortedHypotheses(const Moses::Word &headWord) const;
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

