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
#include "StaticData.h"
#include "ChartHypothesisCollection.h"
#include "ChartHypothesis.h"
#include "ChartManager.h"

using namespace std;
using namespace Moses;

namespace Moses
{

ChartHypothesisCollection::ChartHypothesisCollection()
{
  const StaticData &staticData = StaticData::Instance();

  m_beamWidth = staticData.GetBeamWidth();
  m_maxHypoStackSize = staticData.GetMaxHypoStackSize();
  m_nBestIsEnabled = staticData.IsNBestEnabled();
  m_bestScore = -std::numeric_limits<float>::infinity();
}

ChartHypothesisCollection::~ChartHypothesisCollection()
{
  HCType::iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    ChartHypothesis *hypo = *iter;
    ChartHypothesis::Delete(hypo);
  }
  //RemoveAllInColl(m_hypos);
}

bool ChartHypothesisCollection::AddHypothesis(ChartHypothesis *hypo, ChartManager &manager)
{
  if (hypo->GetTotalScore() < m_bestScore + m_beamWidth) {
    // really bad score. don't bother adding hypo into collection
    manager.GetSentenceStats().AddDiscarded();
    VERBOSE(3,"discarded, too bad for stack" << std::endl);
    ChartHypothesis::Delete(hypo);
    return false;
  }

  // over threshold, try to add to collection
  std::pair<HCType::iterator, bool> addRet = Add(hypo, manager);

  // does it have the same state as an existing hypothesis?
  if (addRet.second) {
    // nothing found. add to collection
    return true;
  }

  // equiv hypo exists, recombine with other hypo
  HCType::iterator &iterExisting = addRet.first;
  ChartHypothesis *hypoExisting = *iterExisting;
  CHECK(iterExisting != m_hypos.end());

  //StaticData::Instance().GetSentenceStats().AddRecombination(*hypo, **iterExisting);

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

    bool added = Add(hypo, manager).second;
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
    } 
    else {
      ChartHypothesis::Delete(hypo);
    }
    return false;
  }
}

pair<ChartHypothesisCollection::HCType::iterator, bool> ChartHypothesisCollection::Add(ChartHypothesis *hypo, ChartManager &manager)
{
  std::pair<HCType::iterator, bool> ret = m_hypos.insert(hypo);
  if (ret.second) {
    // equiv hypo doesn't exists
    VERBOSE(3,"added hyp to stack");

    // Update best score, if this hypothesis is new best
    if (hypo->GetTotalScore() > m_bestScore) {
      VERBOSE(3,", best on stack");
      m_bestScore = hypo->GetTotalScore();
    }

    // Prune only if stack is twice as big as needed (lazy pruning)
    VERBOSE(3,", now size " << m_hypos.size());
    if (m_hypos.size() > 2*m_maxHypoStackSize-1) {
      PruneToSize(manager);
    } else {
      VERBOSE(3,std::endl);
    }
  }

  return ret;
}

/** Remove hypothesis pointed to by iterator but don't delete the object. */
void ChartHypothesisCollection::Detach(const HCType::iterator &iter)
{
  m_hypos.erase(iter);
}

void ChartHypothesisCollection::Remove(const HCType::iterator &iter)
{
  ChartHypothesis *h = *iter;

  /*
   stringstream strme("");
   strme << h->GetOutputPhrase();
   string toFind = "the goal of gene scientists is ";
   size_t pos = toFind.find(strme.str());

   if (pos == 0)
   {
   cerr << pos << " " << strme.str() << *h << endl;
   cerr << *this << endl;
   }
   */

  Detach(iter);
  ChartHypothesis::Delete(h);
}

void ChartHypothesisCollection::PruneToSize(ChartManager &manager)
{
  if (GetSize() > m_maxHypoStackSize) { // ok, if not over the limit
    priority_queue<float> bestScores;

    // push all scores to a heap
    // (but never push scores below m_bestScore+m_beamWidth)
    HCType::iterator iter = m_hypos.begin();
    float score = 0;
    while (iter != m_hypos.end()) {
      ChartHypothesis *hypo = *iter;
      score = hypo->GetTotalScore();
      if (score > m_bestScore+m_beamWidth) {
        bestScores.push(score);
      }
      ++iter;
    }

    // pop the top newSize scores (and ignore them, these are the scores of hyps that will remain)
    //  ensure to never pop beyond heap size
    size_t minNewSizeHeapSize = m_maxHypoStackSize > bestScores.size() ? bestScores.size() : m_maxHypoStackSize;
    for (size_t i = 1 ; i < minNewSizeHeapSize ; i++)
      bestScores.pop();

    // and remember the threshold
    float scoreThreshold = bestScores.top();

    // delete all hypos under score threshold
    iter = m_hypos.begin();
    while (iter != m_hypos.end()) {
      ChartHypothesis *hypo = *iter;
      float score = hypo->GetTotalScore();
      if (score < scoreThreshold) {
        HCType::iterator iterRemove = iter++;
        Remove(iterRemove);
        manager.GetSentenceStats().AddPruning();
      } else {
        ++iter;
      }
    }
    VERBOSE(3,", pruned to size " << m_hypos.size() << endl);

    IFVERBOSE(3) {
      TRACE_ERR("stack now contains: ");
      for(iter = m_hypos.begin(); iter != m_hypos.end(); iter++) {
        ChartHypothesis *hypo = *iter;
        TRACE_ERR( hypo->GetId() << " (" << hypo->GetTotalScore() << ") ");
      }
      TRACE_ERR( endl);
    }

    // desperation pruning
    if (m_hypos.size() > m_maxHypoStackSize * 2) {
      std::vector<ChartHypothesis*> hyposOrdered;

      // sort hypos
      std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(hyposOrdered, hyposOrdered.end()));
      std::sort(hyposOrdered.begin(), hyposOrdered.end(), ChartHypothesisScoreOrderer());

      //keep only |size|. delete the rest
      std::vector<ChartHypothesis*>::iterator iter;
      for (iter = hyposOrdered.begin() + (m_maxHypoStackSize * 2); iter != hyposOrdered.end(); ++iter) {
        ChartHypothesis *hypo = *iter;
        HCType::iterator iterFindHypo = m_hypos.find(hypo);
        CHECK(iterFindHypo != m_hypos.end());
        Remove(iterFindHypo);
      }
    }
  }
}

void ChartHypothesisCollection::SortHypotheses()
{
  CHECK(m_hyposOrdered.empty());
  if (!m_hypos.empty()) {
    // done everything for this cell.
    // sort
    // put into vec
    m_hyposOrdered.reserve(m_hypos.size());
    std::copy(m_hypos.begin(), m_hypos.end(), back_inserter(m_hyposOrdered));
    std::sort(m_hyposOrdered.begin(), m_hyposOrdered.end(), ChartHypothesisScoreOrderer());
  }
}

void ChartHypothesisCollection::CleanupArcList()
{
  HCType::iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    ChartHypothesis *mainHypo = *iter;
    mainHypo->CleanupArcList();
  }
}

void ChartHypothesisCollection::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned, bool> &reachable) const
{
  HCType::const_iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    ChartHypothesis &mainHypo = **iter;
    if (StaticData::Instance().GetUnprunedSearchGraph() ||
        reachable.find(mainHypo.GetId()) != reachable.end()) {
      outputSearchGraphStream << translationId << " " << mainHypo << endl;
    }

    const ChartArcList *arcList = mainHypo.GetArcList();
    if (arcList) {
      ChartArcList::const_iterator iterArc;
      for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
        const ChartHypothesis &arc = **iterArc;
        if (reachable.find(arc.GetId()) != reachable.end()) {
          outputSearchGraphStream << translationId << " " << arc << endl;
        }
      }
    }
  }
}

std::ostream& operator<<(std::ostream &out, const ChartHypothesisCollection &coll)
{
  HypoList::const_iterator iterInside;
  for (iterInside = coll.m_hyposOrdered.begin(); iterInside != coll.m_hyposOrdered.end(); ++iterInside) {
    const ChartHypothesis &hypo = **iterInside;
    out << hypo << endl;
  }

  return out;
}


} // namespace
