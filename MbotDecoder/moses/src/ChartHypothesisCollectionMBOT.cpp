// $Id: ChartHypothesisCollectionMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
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
#include "ChartHypothesisCollectionMBOT.h"
#include "ChartHypothesisMBOT.h"
#include "ChartManager.h"

using namespace std;
using namespace Moses;

namespace Moses
{

ChartHypothesisCollectionMBOT::ChartHypothesisCollectionMBOT()
{
  const StaticData &staticData = StaticData::Instance();

  m_beamWidth = staticData.GetBeamWidth();
  m_maxHypoStackSize = staticData.GetMaxHypoStackSize();
  m_nBestIsEnabled = staticData.IsNBestEnabled();
  m_bestScore = -std::numeric_limits<float>::infinity();
}

ChartHypothesisCollectionMBOT::~ChartHypothesisCollectionMBOT()
{
  HCTypeMBOT::iterator iter;
  for (iter = m_mbotHypos.begin() ; iter != m_mbotHypos.end() ; ++iter) {
    ChartHypothesisMBOT *hypo = *iter;
    ChartHypothesisMBOT::DeleteMBOT(hypo);
  }
  //RemoveAllInColl(m_hypos);
}

bool ChartHypothesisCollectionMBOT::AddHypothesis(ChartHypothesisMBOT *hypo, ChartManager &manager)
{
    //std::cout << "CYADD : ADDING HYPOTHESIS (1) " << *hypo << std::endl;
  if (hypo->GetTotalScore() < m_bestScore + m_beamWidth) {
      //std::cout << "SCORE TOO LOW!" << std::endl;
      //std::cout << "SCORE TOO LOW!" << std::endl;
    // really bad score. don't bother adding hypo into collection
    manager.GetSentenceStats().AddDiscarded();
    //std::cout << "DISCARDED ADDED " << std::endl;
    //VERBOSE(3,"discarded, too bad for stack" << std::endl);
    ChartHypothesisMBOT::DeleteMBOT(hypo);
    //std::cout << "HYPO DELETED " << std::endl;
    return false;
  }

  // over threshold, try to add to collection

  std::pair<HCTypeMBOT::iterator, bool> addRet = Add(hypo, manager);
  //std::cout << "ADDED HYPO TO COLLECTION "<< std::endl;

  // does it have the same state as an existing hypothesis?
  if (addRet.second) {
      //std::cout << "NOTHING FOUND ADD TO COLLECTION" << std::endl;
    // nothing found. add to collection
    return true;
  }

  //std::cout << "Equivalent hypo exists : recombine : " << std::endl;
  // equiv hypo exists, recombine with other hypo
  HCTypeMBOT::iterator &iterExisting = addRet.first;
  ChartHypothesisMBOT *hypoExisting = *iterExisting;
  //std::cout << "Existing hypo : " << *hypoExisting << std::endl;
  CHECK(iterExisting != m_mbotHypos.end());

  //StaticData::Instance().GetSentenceStats().AddRecombination(*hypo, **iterExisting);

  // found existing hypo with same target ending.
  // keep the best 1
  if (hypo->GetTotalScore() > hypoExisting->GetTotalScore()) {
    // incoming hypo is better than the one we have
    //std::cout << "Better than matching hypo!" << std::endl;
    VERBOSE(3,"better than matching hyp " << hypoExisting->GetId() << ", recombining, ");
    if (m_nBestIsEnabled) {
      hypo->AddArc(hypoExisting);
      Detach(iterExisting);
    } else {
      Remove(iterExisting);
    }

    bool added = Add(hypo, manager).second;
    //std::cout << "Value of added " << added << std::endl;
    if (!added) {
      iterExisting = m_mbotHypos.find(hypo);
      TRACE_ERR("Offending hypo = " << **iterExisting << endl);
      abort();
    }
    return false;
  } else {
      //std::cout << "Existing is worse than matching  : " << std::endl;
    //std::cout << "Existing hypo : " << *hypoExisting << std::endl;
    //std::cout << "Current hypo : " << *hypoExisting << std::endl;
    // already storing the best hypo. discard current hypo
    VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
    if (m_nBestIsEnabled) {
      //std::cout << "Adding Arc for hypo"<< std::endl;
      hypoExisting->AddArc(hypo);
    }
    else {
      //std::cout << "DELETING HYPO after added to arc" << std::endl;
      ChartHypothesisMBOT::DeleteMBOT(hypo);
    }
    return false;
  }
}

pair<ChartHypothesisCollectionMBOT::HCTypeMBOT::iterator, bool> ChartHypothesisCollectionMBOT::Add(ChartHypothesisMBOT *hypo, ChartManager &manager)
{
  //std::cout << "CYADD : ADDING HYPOTHESIS (2) : " << (*hypo) << std::endl;

  std::pair<HCTypeMBOT::iterator, bool> ret = m_mbotHypos.insert(hypo);
  if (ret.second) {
    //std::cout << "No equivalent hypo exists " << std::endl;
    // equiv hypo doesn't exists
    VERBOSE(3,"added hyp to stack");
    //std::cout << "ADDED TO STACK" << std::endl;
    // Update best score, if this hypothesis is new best
    if (hypo->GetTotalScore() > m_bestScore) {
        //std::cout << "Hypo is new best " << std::endl;
      VERBOSE(3,", best on stack");
      //std::cout << "BEST ON STACK" << std::endl;
      m_bestScore = hypo->GetTotalScore();
    }

    // Prune only if stack is twice as big as needed (lazy pruning)
    VERBOSE(3,", now size " << m_mbotHypos.size());
    //std::cout << "NEW SIZE" << m_mbotHypos.size();
    if (m_mbotHypos.size() > 2*m_maxHypoStackSize-1) {
      //std::cout << "TO BIG : PRUNE" << std::endl;
      PruneToSize(manager);
    } else {
      VERBOSE(3,std::endl);
      //std::cout << std::endl;

    }
  }
  //std::cout << "EQUIVALENT HYPO FOUND : not inserted" << std::endl;
  return ret;
}

/** Remove hypothesis pointed to by iterator but don't delete the object. */
void ChartHypothesisCollectionMBOT::Detach(const HCTypeMBOT::iterator &iter)
{
  m_mbotHypos.erase(iter);
}

void ChartHypothesisCollectionMBOT::Remove(const HCTypeMBOT::iterator &iter)
{
  ChartHypothesisMBOT *h = *iter;

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
  ChartHypothesisMBOT::DeleteMBOT(h);
}

void ChartHypothesisCollectionMBOT::PruneToSize(ChartManager &manager)
{
  //std::cout << "CYPRUNE : PRUNING COLLECTION" << std::endl;
  if (GetSizeMBOT() > m_maxHypoStackSize) { // ok, if not over the limit
    priority_queue<float> bestScores;

    // push all scores to a heap
    // (but never push scores below m_bestScore+m_beamWidth)
    HCTypeMBOT::iterator iter = m_mbotHypos.begin();
    float score = 0;
    while (iter != m_mbotHypos.end()) {
      ChartHypothesisMBOT *hypo = *iter;
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
    iter = m_mbotHypos.begin();
    while (iter != m_mbotHypos.end()) {
      ChartHypothesisMBOT *hypo = *iter;
      float score = hypo->GetTotalScore();
      if (score < scoreThreshold) {
        HCTypeMBOT::iterator iterRemove = iter++;
        Remove(iterRemove);
        manager.GetSentenceStats().AddPruning();
      } else {
        ++iter;
      }
    }
    VERBOSE(3,", pruned to size " << m_mbotHypos.size() << endl);

    IFVERBOSE(3) {
      TRACE_ERR("stack now contains: ");
      for(iter = m_mbotHypos.begin(); iter != m_mbotHypos.end(); iter++) {
        ChartHypothesisMBOT *hypo = *iter;
        TRACE_ERR( hypo->GetId() << " (" << hypo->GetTotalScore() << ") ");
      }
      TRACE_ERR( endl);
    }

    // desperation pruning
    if (m_mbotHypos.size() > m_maxHypoStackSize * 2) {
      std::vector<ChartHypothesisMBOT*> hyposOrdered;

      // sort hypos
      std::copy(m_mbotHypos.begin(), m_mbotHypos.end(), std::inserter(hyposOrdered, hyposOrdered.end()));
      std::sort(hyposOrdered.begin(), hyposOrdered.end(), ChartHypothesisScoreOrdererMBOT());

      //keep only |size|. delete the rest
      std::vector<ChartHypothesisMBOT*>::iterator iter;
      for (iter = hyposOrdered.begin() + (m_maxHypoStackSize * 2); iter != hyposOrdered.end(); ++iter) {
        ChartHypothesisMBOT *hypo = *iter;
        HCTypeMBOT::iterator iterFindHypo = m_mbotHypos.find(hypo);
        CHECK(iterFindHypo != m_mbotHypos.end());
        Remove(iterFindHypo);
      }
    }
  }
}

void ChartHypothesisCollectionMBOT::SortHypotheses()
{
  //std::cout << "SORT SORT SORT" << std::endl;
  CHECK(m_mbotHyposOrdered.empty());
  //std::cout << "NON ordered MBOT Hypos : " << m_mbotHypos.size() << std::endl;
  //ChartHypothesisCollectionMBOT::HCTypeMBOT::const_iterator iter;
  //for(iter = m_mbotHypos.begin(); iter != m_mbotHypos.end(); iter++)
  //{
  //const ChartHypothesisMBOT &testHypo = **iter;
      //std::cout << "HYPO : " << testHypo << std::endl;
  //}

  if (!m_mbotHypos.empty()) {
    // done everything for this cell.
    // sort
    // put into vec
    m_mbotHyposOrdered.reserve(m_mbotHypos.size());
    std::copy(m_mbotHypos.begin(), m_mbotHypos.end(), back_inserter(m_mbotHyposOrdered));
    //std::cout << "Size after copy : " << m_mbotHyposOrdered.size() << " : " << *m_mbotHyposOrdered.front() << std::endl;

    std::sort(m_mbotHyposOrdered.begin(), m_mbotHyposOrdered.end(), ChartHypothesisScoreOrdererMBOT());
  }
}

void ChartHypothesisCollectionMBOT::CleanupArcList()
{
  HCTypeMBOT::iterator iter;
  //std::cout << "Size of Collection : " << m_mbotHypos.size() << std::endl;
  for (iter = m_mbotHypos.begin() ; iter != m_mbotHypos.end() ; ++iter) {
    ChartHypothesisMBOT *mainHypo = *iter;
    //std::cout << "Main Hypo : " << (*mainHypo) << std::endl;
    mainHypo->CleanupArcList();
  }
}

void ChartHypothesisCollectionMBOT::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned, bool> &reachable) const
{
  HCTypeMBOT::const_iterator iter;
  for (iter = m_mbotHypos.begin() ; iter != m_mbotHypos.end() ; ++iter) {
    ChartHypothesisMBOT &mainHypo = **iter;
    if (StaticData::Instance().GetUnprunedSearchGraph() ||
        reachable.find(mainHypo.GetId()) != reachable.end()) {
      outputSearchGraphStream << translationId << " " << mainHypo << endl;
    }

    const ChartArcListMBOT *arcList = mainHypo.GetArcListMBOT();
    if (arcList) {
      ChartArcListMBOT::const_iterator iterArc;
      for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
        const ChartHypothesisMBOT &arc = **iterArc;
        if (reachable.find(arc.GetId()) != reachable.end()) {
          outputSearchGraphStream << translationId << " " << arc << endl;
        }
      }
    }
  }
}

std::ostream& operator<<(std::ostream &out, const ChartHypothesisCollectionMBOT &coll)
{
  HypoListMBOT::const_iterator iterInside;
  ChartHypothesisCollectionMBOT::iterator itrSet;

  if(coll.GetSizeMBOT() == 0){std::cout << "EMPTY COLLECTION" << std::endl;}
  else{
      std::cout << "Size of collection : " << coll.GetHypoMBOT().size() << std::endl;
      std::cout << "Size of sorted collection : " << coll.GetSortedHypothesesMBOT().size() << std::endl;

  std::cout << "PRINTING UNSORTED COLLECTION" << std::endl;
  for(itrSet = coll.begin(); itrSet != coll.end(); itrSet++)
  {
      const ChartHypothesisMBOT * hypo = *itrSet;
      out << (*hypo) << endl;
  }

  std::cout << "PRINTING SORTED COLLECTION : " << std::endl;
  for (iterInside = coll.GetSortedHypothesesMBOT().begin(); iterInside != coll.GetSortedHypothesesMBOT().end(); iterInside++) {
    const ChartHypothesisMBOT * hypo = *iterInside;
    out << (*hypo) << endl;
  }
  }
  //std::cout << "RETURNING OUTPUT " << std::endl;
  return out;
}

} // namespace
