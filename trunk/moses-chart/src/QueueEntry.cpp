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

#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "Cube.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/ChartTranslationOption.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/WordConsumed.h"

#ifdef HAVE_BOOST
#include <boost/functional/hash.hpp>
#endif

using namespace std;
using namespace Moses;

namespace MosesChart
{

QueueEntry::QueueEntry(const Moses::ChartTranslationOption &transOpt
											 , const ChartCellCollection &allChartCells)
:m_transOpt(transOpt)
{
	const WordConsumed *wordsConsumed = &transOpt.GetLastWordConsumed();
	CreateChildEntry(wordsConsumed, allChartCells);
	CalcScore();
}

void QueueEntry::CreateChildEntry(const Moses::WordConsumed *wordsConsumed, const ChartCellCollection &allChartCells)
{
	// recursvile do the 1st first
	const WordConsumed *prevWordsConsumed = wordsConsumed->GetPrevWordsConsumed();
	if (prevWordsConsumed)
		CreateChildEntry(prevWordsConsumed, allChartCells);

	if (wordsConsumed->IsNonTerminal())
	{ // non-term
		const WordsRange &childRange = wordsConsumed->GetWordsRange();
		const ChartCell &childCell = allChartCells.Get(childRange);
		const Word &headWord = wordsConsumed->GetSourceWord();

		assert(!childCell.GetSortedHypotheses(headWord).empty());

		const Moses::Word &nonTerm = wordsConsumed->GetSourceWord();
		assert(nonTerm.IsNonTerminal());
		ChildEntry childEntry(0, childCell.GetSortedHypotheses(nonTerm), nonTerm);
		m_childEntries.push_back(childEntry);
	}
}

QueueEntry::QueueEntry(const QueueEntry &copy, size_t childEntryIncr)
:m_transOpt(copy.m_transOpt)
,m_childEntries(copy.m_childEntries)
{
	ChildEntry &childEntry = m_childEntries[childEntryIncr];
	childEntry.IncrementPos();
	CalcScore();
}

QueueEntry::~QueueEntry()
{
	//Moses::RemoveAllInColl(m_childEntries);
}

void QueueEntry::CreateDeviants(Cube &cube) const
{
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = m_childEntries[ind];

		if (childEntry.HasMoreHypo())
		{
			QueueEntry *newEntry = new QueueEntry(*this, ind);
			cube.Add(newEntry);
		}
	}
}

void QueueEntry::CalcScore()
{
	m_combinedScore = m_transOpt.GetTotalScore();
	for (size_t ind = 0; ind < m_childEntries.size(); ind++)
	{
		const ChildEntry &childEntry = m_childEntries[ind];

		const Hypothesis *hypo = childEntry.GetHypothesis();
		m_combinedScore += hypo->GetTotalScore();
	}

}

bool QueueEntry::operator<(const QueueEntry &compare) const
{ 
	if (&m_transOpt != &compare.m_transOpt)
		return &m_transOpt < &compare.m_transOpt;
	
	bool ret = m_childEntries < compare.m_childEntries;
	return ret;
}

#ifdef HAVE_BOOST
std::size_t hash_value(const ChildEntry & entry)
{
    boost::hash<const Hypothesis*> hasher;
    return hasher(entry.GetHypothesis());
}

#endif
std::ostream& operator<<(std::ostream &out, const ChildEntry &entry)
{
	out << *entry.GetHypothesis();
	return out;
}

std::ostream& operator<<(std::ostream &out, const QueueEntry &entry)
{
	out << entry.GetTranslationOption() << endl;
	std::vector<ChildEntry>::const_iterator iter;
	for (iter = entry.GetChildEntries().begin(); iter != entry.GetChildEntries().end(); ++iter)
	{
		out << *iter << endl;
	}
	return out;
}

}

