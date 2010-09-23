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

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <iostream>
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/Word.h"
#include "ChartHypothesis.h"

namespace Moses
{
	class WordConsumed;
	class ChartTranslationOption;
	extern bool g_debug;
};

namespace MosesChart
{

class TranslationOptionCollection;
class TranslationOptionList;
class ChartCell;
class ChartCellCollection;
class QueueEntry;
class Cube;

typedef std::vector<const Hypothesis*> HypoList;

	// wrapper around list of hypothese for a particular non-term of a trans opt
class ChildEntry
{
	friend std::ostream& operator<<(std::ostream&, const ChildEntry&);

protected:
	size_t m_pos;
	const HypoList *m_orderedHypos;

public:
  ChildEntry(size_t pos, const HypoList &orderedHypos, const Moses::Word & /*headWord*/ )
		:m_pos(pos)
		,m_orderedHypos(&orderedHypos)
		//,m_headWord(headWord)
	{}

	size_t IncrementPos()
	{ return m_pos++; }
	
	bool HasMoreHypo() const
	{
		return m_pos + 1 < m_orderedHypos->size();	
	}
	
	const Hypothesis *GetHypothesis() const
	{ 		
		return (*m_orderedHypos)[m_pos]; 
	}
	
	//const Moses::Word &GetHeadWord() const
	//{ return m_headWord; }

	//! transitive comparison used for adding objects into FactorCollection
	bool operator<(const ChildEntry &compare) const
	{ 
		return GetHypothesis() < compare.GetHypothesis();
	}
};

// entry in the cub of 1 trans opt and all the hypotheses that goes with each non term.
class QueueEntry
{
	friend std::ostream& operator<<(std::ostream&, const QueueEntry&);
protected:
	const Moses::ChartTranslationOption &m_transOpt;
	std::vector<ChildEntry> m_childEntries;

	float m_combinedScore;

	QueueEntry(const QueueEntry &copy, size_t childEntryIncr);
	bool CreateChildEntry(const Moses::WordConsumed *wordsConsumed, const ChartCellCollection &allChartCells);

	void CalcScore();

public:
	QueueEntry(const Moses::ChartTranslationOption &transOpt
						, const ChartCellCollection &allChartCells
						, bool &isOK);
	~QueueEntry();

	const Moses::ChartTranslationOption &GetTranslationOption() const
	{ return m_transOpt; }
	const std::vector<ChildEntry> &GetChildEntries() const
	{ return m_childEntries; }
	float GetCombinedScore() const
	{ return m_combinedScore; }

	void CreateDeviants(Cube &) const;

	bool operator<(const QueueEntry &compare) const;
	
};

}
