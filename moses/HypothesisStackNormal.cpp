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
#include "HypothesisStackNormal.h"
#include "TypeDef.h"
#include "Util.h"
#include "StaticData.h"
#include "Manager.h"

using namespace std;

namespace Moses
{
HypothesisStackNormal::HypothesisStackNormal(Manager& manager) :
  HypothesisStack(manager)
{
  m_nBestIsEnabled = StaticData::Instance().IsNBestEnabled();
  m_bestScore = -std::numeric_limits<float>::infinity();
  m_worstScore = -std::numeric_limits<float>::infinity();
}

/** remove all hypotheses from the collection */
void HypothesisStackNormal::RemoveAll()
{
  while (m_hypos.begin() != m_hypos.end()) {
    Remove(m_hypos.begin());
  }
}

pair<HypothesisStackNormal::iterator, bool> HypothesisStackNormal::Add(Hypothesis *hypo)
{
  std::pair<iterator, bool> ret = m_hypos.insert(hypo);
  if (ret.second) {
    // equiv hypo doesn't exists
    VERBOSE(3,"added hyp to stack");

    // Update best score, if this hypothesis is new best
    if (hypo->GetTotalScore() > m_bestScore) {
      VERBOSE(3,", best on stack");
      m_bestScore = hypo->GetTotalScore();
      // this may also affect the worst score
      if ( m_bestScore + m_beamWidth > m_worstScore )
        m_worstScore = m_bestScore + m_beamWidth;
    }
    // update best/worst score for stack diversity 1
    if ( m_minHypoStackDiversity == 1 &&
         hypo->GetTotalScore() > GetWorstScoreForBitmap( hypo->GetWordsBitmap() ) ) {
      SetWorstScoreForBitmap( hypo->GetWordsBitmap().GetID(), hypo->GetTotalScore() );
    }

    VERBOSE(3,", now size " << m_hypos.size());

    // prune only if stack is twice as big as needed (lazy pruning)
    size_t toleratedSize = 2*m_maxHypoStackSize-1;
    // add in room for stack diversity
    if (m_minHypoStackDiversity)
      toleratedSize += m_minHypoStackDiversity << StaticData::Instance().GetMaxDistortion();
    if (m_hypos.size() > toleratedSize) {
      PruneToSize(m_maxHypoStackSize);
    } else {
      VERBOSE(3,std::endl);
    }
  }

  return ret;
}

bool HypothesisStackNormal::AddPrune(Hypothesis *hypo)
{
  if (hypo->GetTotalScore() == - std::numeric_limits<float>::infinity()) {
    m_manager.GetSentenceStats().AddDiscarded();
    VERBOSE(3,"discarded, constraint" << std::endl);
    FREEHYPO(hypo);
    return false;
  }

  // too bad for stack. don't bother adding hypo into collection
  if (!StaticData::Instance().GetDisableDiscarding() &&
      hypo->GetTotalScore() < m_worstScore
      && ! ( m_minHypoStackDiversity > 0
             && hypo->GetTotalScore() >= GetWorstScoreForBitmap( hypo->GetWordsBitmap() ) ) ) {
    m_manager.GetSentenceStats().AddDiscarded();
    VERBOSE(3,"discarded, too bad for stack" << std::endl);
    FREEHYPO(hypo);
    return false;
  }

  // over threshold, try to add to collection
  std::pair<iterator, bool> addRet = Add(hypo);
  if (addRet.second) {
    // nothing found. add to collection
    return true;
  }

  // equiv hypo exists, recombine with other hypo
  iterator &iterExisting = addRet.first;
  Hypothesis *hypoExisting = *iterExisting;
  assert(iterExisting != m_hypos.end());

  m_manager.GetSentenceStats().AddRecombination(*hypo, **iterExisting);

  // found existing hypo with same target ending.
  // keep the best 1
  if (hypo->GetTotalScore() > hypoExisting->GetTotalScore()) {
    // incoming hypo is better than the one we have
    VERBOSE(3,"better than matching hyp " << hypoExisting->GetId() << ", recombining, ");
    if (m_nBestIsEnabled) {
      hypo->AddArc(hypoExisting);
      Detach(iterExisting);
    } else {
      Remove(iterExisting);
    }

    bool added = Add(hypo).second;
    if (!added) {
      iterExisting = m_hypos.find(hypo);
      TRACE_ERR("Offending hypo = " << **iterExisting << endl);
      abort();
    }
    return false;
  } else {
    // already storing the best hypo. discard current hypo
    VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
    if (m_nBestIsEnabled) {
      hypoExisting->AddArc(hypo);
    } else {
      FREEHYPO(hypo);
    }
    return false;
  }
}

void HypothesisStackNormal::PruneToSize(size_t newSize)
{
  if ( size() <= newSize ) return; // ok, if not over the limit

  // we need to store a temporary list of hypotheses
  vector< Hypothesis* > hypos = GetSortedListNOTCONST();
  bool* included = (bool*) malloc(sizeof(bool) * hypos.size());
  for(size_t i=0; i<hypos.size(); i++) included[i] = false;

  // clear out original set
  for( iterator iter = m_hypos.begin(); iter != m_hypos.end(); ) {
    iterator removeHyp = iter++;
    Detach(removeHyp);
  }

  // add best hyps for each coverage according to minStackDiversity
  if ( m_minHypoStackDiversity > 0 ) {
    map< WordsBitmapID, size_t > diversityCount;
    for(size_t i=0; i<hypos.size(); i++) {
      Hypothesis *hyp = hypos[i];
      WordsBitmapID coverage = hyp->GetWordsBitmap().GetID();;
      if (diversityCount.find( coverage ) == diversityCount.end())
        diversityCount[ coverage ] = 0;

      if (diversityCount[ coverage ] < m_minHypoStackDiversity) {
        m_hypos.insert( hyp );
        included[i] = true;
        diversityCount[ coverage ]++;
        if (diversityCount[ coverage ] == m_minHypoStackDiversity)
          SetWorstScoreForBitmap( coverage, hyp->GetTotalScore());
      }
    }
  }

  // only add more if stack not full after satisfying minStackDiversity
  if ( size() < newSize ) {

    // add best remaining hypotheses
    for(size_t i=0; i<hypos.size()
        && size() < newSize
        && hypos[i]->GetTotalScore() > m_bestScore+m_beamWidth; i++) {
      if (! included[i]) {
        m_hypos.insert( hypos[i] );
        included[i] = true;
        if (size() == newSize)
          m_worstScore = hypos[i]->GetTotalScore();
      }
    }
  }

  // delete hypotheses that have not been included
  for(size_t i=0; i<hypos.size(); i++) {
    if (! included[i]) {
      FREEHYPO( hypos[i] );
      m_manager.GetSentenceStats().AddPruning();
    }
  }
  free(included);

  // some reporting....
  VERBOSE(3,", pruned to size " << size() << endl);
  IFVERBOSE(3) {
    TRACE_ERR("stack now contains: ");
    for(iterator iter = m_hypos.begin(); iter != m_hypos.end(); iter++) {
      Hypothesis *hypo = *iter;
      TRACE_ERR( hypo->GetId() << " (" << hypo->GetTotalScore() << ") ");
    }
    TRACE_ERR( endl);
  }
}

const Hypothesis *HypothesisStackNormal::GetBestHypothesis() const
{
  if (!m_hypos.empty()) {
    const_iterator iter = m_hypos.begin();
    Hypothesis *bestHypo = *iter;
    while (++iter != m_hypos.end()) {
      Hypothesis *hypo = *iter;
      if (hypo->GetTotalScore() > bestHypo->GetTotalScore())
        bestHypo = hypo;
    }
    return bestHypo;
  }
  return NULL;
}

vector<const Hypothesis*> HypothesisStackNormal::GetSortedList() const
{
  vector<const Hypothesis*> ret;
  ret.reserve(m_hypos.size());
  std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
  sort(ret.begin(), ret.end(), CompareHypothesisTotalScore());

  return ret;
}

vector<Hypothesis*> HypothesisStackNormal::GetSortedListNOTCONST()
{
  vector<Hypothesis*> ret;
  ret.reserve(m_hypos.size());
  std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
  sort(ret.begin(), ret.end(), CompareHypothesisTotalScore());

  return ret;
}

void HypothesisStackNormal::CleanupArcList()
{
  // only necessary if n-best calculations are enabled
  if (!m_nBestIsEnabled) return;

  iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    Hypothesis *mainHypo = *iter;
    mainHypo->CleanupArcList();
  }
}

TO_STRING_BODY(HypothesisStackNormal);


// friend
std::ostream& operator<<(std::ostream& out, const HypothesisStackNormal& hypoColl)
{
  HypothesisStackNormal::const_iterator iter;

  for (iter = hypoColl.begin() ; iter != hypoColl.end() ; ++iter) {
    const Hypothesis &hypo = **iter;
    out << hypo << endl;

  }
  return out;
}


}

