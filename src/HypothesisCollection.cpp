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
#include <queue>
#include "HypothesisCollection.h"
#include "TypeDef.h"
#include "Util.h"

using namespace std;

size_t CompareHypothesisCollection::s_ngramMaxOrder[NUM_FACTORS] = {0,0,0,0};
	// need to change if we add more factors, or use a macro

void HypothesisCollection::RemoveAll()
{
	while (m_hypos.begin() != m_hypos.end())
	{
		Remove(m_hypos.begin());
	}
}
 
void HypothesisCollection::Add(Hypothesis *hypo)
{

	AddNoPrune(hypo);

	if (hypo->GetScore(ScoreType::Total) > m_bestScore)
	{
		m_bestScore = hypo->GetScore(ScoreType::Total);
        if ( m_bestScore + m_beamThreshold > m_worstScore )
          m_worstScore = m_bestScore + m_beamThreshold;
	}

    // Prune only of stack is twice as big as needed
	if (m_hypos.size() > 2*m_maxHypoStackSize-10)
	{
		PruneToSize(m_maxHypoStackSize);
	}
}


float HypothesisCollection::getBestScore() const{
	return m_bestScore;
}



bool HypothesisCollection::AddPrune(Hypothesis *hypo)
{
	if (hypo->GetScore(ScoreType::Total) < m_worstScore)
		return false;

	// over threshold		
	// recombine if ngram-equivalent to another hypo
	iterator iter = m_hypos.find(hypo);
	if (iter == m_hypos.end())
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
	if (m_hypos.size() > newSize)
	{
        // Pruning alg: find a threshold and delete all hypothesis below it
        //   the threshold is chosen so that exactly newSize top items remain on the stack
        //   in fact, in situations where some of the hypothesis fell below m_beamThreshold,
        //   the stack will contain less items

		priority_queue<float> bestScores;

        // cerr << "About to prune from " << size() << " to " << newSize << endl;
        // push all scores to a heap
        //   (but never push scores below m_bestScore+m_beamThreshold)
		iterator iter = m_hypos.begin();
        float score = 0;
		while (iter != m_hypos.end())
		{
			Hypothesis *hypo = *iter;
			score = hypo->GetScore(ScoreType::Total);
            // cerr << "H score: " << score << ", mbestscore: " << m_bestScore << " + m_beamThreshold "<< m_beamThreshold << " = " << m_bestScore+m_beamThreshold;
            if (score > m_bestScore+m_beamThreshold) {
			  bestScores.push(score);
              // cerr << " pushed.";
            }
            // cerr << endl;
            ++iter;
        }
        // cerr << "Heap contains " << bestScores.size() << " items" << endl;
        
        // pop the top newSize scores (and ignore them, these are the scores of hyps that will remain)
        //  ensure to never pop beyond heap size
        size_t minNewSizeHeapSize = newSize > bestScores.size() ? bestScores.size() : newSize;
		for (size_t i = 1 ; i < minNewSizeHeapSize ; i++)
          bestScores.pop();

        // cerr << "Popped "<< newSize << ", heap now contains " << bestScores.size() << " items" << endl;

        // and remember the threshold
        float scoreThreshold = bestScores.top();
        // cerr << "threshold: " << scoreThreshold << endl;

		// delete all hypos under score threshold
		iter = m_hypos.begin();
		while (iter != m_hypos.end())
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
        // cerr << "Stack size after pruning: " << size() << endl;

        // set the worstScore, so that newly generated hypotheses will not be added if worse than the worst in the stack
        m_worstScore = scoreThreshold;
	}
}

const Hypothesis *HypothesisCollection::GetBestHypothesis() const
{
	if (!m_hypos.empty())
	{
		const_iterator iter = m_hypos.begin();
		Hypothesis *bestHypo = *iter;
		while (++iter != m_hypos.end())
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
	std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
	ret.sort(HypothesisSortDescending());

	return ret;
}


void HypothesisCollection::InitializeArcs()
{
#ifdef N_BEST
	iterator iter;
	for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter)
	{
		Hypothesis *mainHypo = *iter;
		mainHypo->InitializeArcs();
	}
#endif
}

TO_STRING_BODY(HypothesisCollection);


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

