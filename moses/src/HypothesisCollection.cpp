// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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
#include <set>
#include "HypothesisCollection.h"
#include "TypeDef.h"
#include "Util.h"

using namespace std;

size_t CompareHypothesisCollection::m_NGramMaxOrder;

void HypothesisCollection::RemoveAll()
{
	while (begin() != end())
	{
		Remove(begin());
	}
}
 
void HypothesisCollection::Add(Hypothesis *hypo)
{
	AddNoPrune(hypo);

	if (hypo->GetScore(ScoreType::Total) > m_bestScore)
	{
		m_bestScore = hypo->GetScore(ScoreType::Total);
	}

	if (size() > m_maxHypoStackSize)
	{
		PruneToSize(m_maxHypoStackSize);
	}
}


float HypothesisCollection::getBestScore(){
	return m_bestScore;
}



bool HypothesisCollection::Add(Hypothesis *hypo, float beamThreshold)
{
	if (hypo->GetScore(ScoreType::Total) < m_bestScore + beamThreshold)
		return false;

	// over threshold		
	// recombine if ngram-equivalent to another hypo
	iterator iter = find(hypo);
	if (iter == end())
	{ // nothing found. add to collection
		Add(hypo);
		return true;
	}
	
	// found existing hypo with same target ending.
	// keep the best 1
	Hypothesis *hypoExisting = *iter;
	if (hypo->GetScore(ScoreType::Total) > hypoExisting->GetScore(ScoreType::Total))
	{
#ifdef N_BEST
		hypo->AddArc(**iter);
#endif
		Remove(iter);
		Add(hypo);
		return true;
	}
	else
	{ // already storing the best hypo. discard current hypo 
#ifdef N_BEST
		(*iter)->AddArc(*hypo);
#endif
		return false;
	}
}

void HypothesisCollection::PruneToSize(size_t newSize)
{
	if (size() > newSize)
	{
		multiset<float> bestScores;

		iterator iter = begin();
		// fill up set to newSize
		for (size_t i = 0 ; i < newSize ; i++)
		{
			Hypothesis *hypo = *iter;
			bestScores.insert(hypo->GetScore(ScoreType::Total));
			++iter;
		}
		// only add score if better than score threshold
		float scoreThreshold = *bestScores.begin();
		while (iter != end())
		{
			Hypothesis *hypo = *iter;
			float score = hypo->GetScore(ScoreType::Total);
			if (score > scoreThreshold)
			{
				bestScores.insert(score);
				bestScores.erase(bestScores.begin());
				scoreThreshold = *bestScores.begin();
			}
			++iter;
		}
		// delete hypo under score threshold
		iter = begin();
		while (iter != end())
		{
			Hypothesis *hypo = *iter;
			float score = hypo->GetScore(ScoreType::Total);
			if (score < scoreThreshold)
			{
				iterator iterRemove = iter++;
				Remove(iterRemove);
			}
			else
			{
				++iter;
			}
		}
	}
}

const Hypothesis *HypothesisCollection::GetBestHypothesis() const
{
	if (!empty())
	{
		const_iterator iter = begin();
		Hypothesis *bestHypo = *iter;
		while (++iter != end())
		{
			Hypothesis *hypo = *iter;
			if (hypo->GetScore(ScoreType::Total) > bestHypo->GetScore(ScoreType::Total))
				bestHypo = hypo;
		}
		return bestHypo;
	}
	return NULL;
}

// sorting helper
struct HypothesisSortDescending
{
	bool operator()(const Hypothesis*& hypo1, const Hypothesis*& hypo2)
	{
		return hypo1->GetScore(ScoreType::Total) > hypo2->GetScore(ScoreType::Total);
	}
};

list<const Hypothesis*> HypothesisCollection::GetSortedList() const
{
	list<const Hypothesis*> ret;
	std::copy(begin(), end(), std::inserter(ret, ret.end()));
	ret.sort(HypothesisSortDescending());

	return ret;
}

void HypothesisCollection::InitializeArcs()
{
#ifdef N_BEST
	iterator iter;
	for (iter = begin() ; iter != end() ; ++iter)
	{
		Hypothesis *mainHypo = *iter;
		mainHypo->InitializeArcs();
	}
#endif
}

// friend
std::ostream& operator<<(std::ostream& out, const HypothesisCollection& hypoColl)
{
	HypothesisCollection::const_iterator iter;
	
	for (iter = hypoColl.begin() ; iter != hypoColl.end() ; ++iter)
	{
		const Hypothesis &hypo = **iter;
		out << hypo << endl;
		
	}
	return out;
}

