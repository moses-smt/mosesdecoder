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
#include "StaticData.h"

using namespace std;

HypothesisCollection::HypothesisCollection()
{
	m_nBestIsEnabled = StaticData::Instance()->IsNBestEnabled();
	m_bestScore = -std::numeric_limits<float>::infinity();
	m_worstScore = -std::numeric_limits<float>::infinity();
}

/** remove all hypotheses from the collection */
void HypothesisCollection::RemoveAll()
{
	while (m_hypos.begin() != m_hypos.end())
	{
		Remove(m_hypos.begin());
	}
}

/** add a hypothesis to the collection, prune if necessary */
void HypothesisCollection::Add(Hypothesis *hypo)
{
	AddNoPrune(hypo);
	VERBOSE(3,"added hyp to stack");

	// Update best score, if this hypothesis is new best
	if (hypo->GetTotalScore() > m_bestScore)
	{
		VERBOSE(3,", best on stack");
		m_bestScore = hypo->GetTotalScore();
		// this may also affect the worst score
        if ( m_bestScore + m_beamThreshold > m_worstScore )
          m_worstScore = m_bestScore + m_beamThreshold;
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

/** add hypothesis to stack (unless worse than minimum score) */
void HypothesisCollection::AddPrune(Hypothesis *hypo)
{ // if returns false, hypothesis not used
	// caller must take care to delete unused hypo to avoid leak

	if (hypo->GetTotalScore() < m_worstScore)
	{ // really bad score. don't bother adding hypo into collection
	  StaticData::Instance()->GetSentenceStats().AddDiscarded();
	  VERBOSE(3,"discarded, too bad for stack" << std::endl);
		ObjectPool<Hypothesis> &pool = Hypothesis::GetObjectPool();
		pool.freeObject(hypo);		
		return;
	}

	// over threshold		
	// recombine if ngram-equivalent to another hypo
	iterator iter = m_hypos.find(hypo);
	if (iter == m_hypos.end())
	{ // nothing found. add to collection
		Add(hypo);
		return;
  }

	StaticData::Instance()->GetSentenceStats().AddRecombination(*hypo, **iter);
	
	// found existing hypo with same target ending.
	// keep the best 1
	Hypothesis *hypoExisting = *iter;
	if (hypo->GetTotalScore() > hypoExisting->GetTotalScore())
	{ // incoming hypo is better than the one we have
		VERBOSE(3,"better than matching hyp " << hypoExisting->GetId() << ", recombining, ");
		if (m_nBestIsEnabled) {
			hypo->AddArc(hypoExisting);
			Detach(iter);
		} else {
			Remove(iter);
		}
		Add(hypo);		
		return;
	}
	else
	{ // already storing the best hypo. discard current hypo 
	  VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
		if (m_nBestIsEnabled) {
			(*iter)->AddArc(hypo);
		} else {
			ObjectPool<Hypothesis> &pool = Hypothesis::GetObjectPool();
			pool.freeObject(hypo);				
		}
		return;
	}
}

/** pruning, if too large.
 * Pruning algorithm: find a threshold and delete all hypothesis below it.
 * The threshold is chosen so that exactly newSize top items remain on the 
 * stack in fact, in situations where some of the hypothesis fell below 
 * m_beamThreshold, the stack will contain less items.
 * \param newSize maximum size */

void HypothesisCollection::PruneToSize(size_t newSize)
{
	if (m_hypos.size() > newSize) // ok, if not over the limit
		{
			priority_queue<float> bestScores;
			
			// push all scores to a heap
			// (but never push scores below m_bestScore+m_beamThreshold)
			iterator iter = m_hypos.begin();
			float score = 0;
			while (iter != m_hypos.end())
				{
					Hypothesis *hypo = *iter;
					score = hypo->GetTotalScore();
					if (score > m_bestScore+m_beamThreshold) {
						bestScores.push(score);
					}
					++iter;
        }
			
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
					float score = hypo->GetTotalScore();
					if (score < scoreThreshold)
						{
							iterator iterRemove = iter++;
							Remove(iterRemove);
							StaticData::Instance()->GetSentenceStats().AddPruning();
						}
					else
						{
							++iter;
						}
				}
			VERBOSE(3,", pruned to size " << size() << endl);
			
			IFVERBOSE(3) {
				cerr << "stack now contains: ";
				for(iter = m_hypos.begin(); iter != m_hypos.end(); iter++) 
					{
						Hypothesis *hypo = *iter;
						cerr << hypo->GetId() << " (" << hypo->GetTotalScore() << ") ";
					}
				cerr << endl;
			}

			// set the worstScore, so that newly generated hypotheses will not be added if worse than the worst in the stack
			m_worstScore = scoreThreshold;
			// cerr << "Heap contains " << bestScores.size() << " items" << endl;
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
			if (hypo->GetTotalScore() > bestHypo->GetTotalScore())
				bestHypo = hypo;
		}
		return bestHypo;
	}
	return NULL;
}

// sorting helper
struct HypothesisSortDescending
{
	const bool operator()(const Hypothesis* hypo1, const Hypothesis* hypo2) const
	{
		return hypo1->GetTotalScore() > hypo2->GetTotalScore();
	}
};

vector<const Hypothesis*> HypothesisCollection::GetSortedList() const
{
	vector<const Hypothesis*> ret; ret.reserve(m_hypos.size());
	std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
	sort(ret.begin(), ret.end(), HypothesisSortDescending());

	return ret;
}


void HypothesisCollection::InitializeArcs()
{
	// only necessary if n-best calculations are enabled
	if (!m_nBestIsEnabled) return;

	iterator iter;
	for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter)
	{
		Hypothesis *mainHypo = *iter;
		mainHypo->InitializeArcs();
	}
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

