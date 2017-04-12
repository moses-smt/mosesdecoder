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
#include "HypothesisStackCubePruning.h"
#include "TypeDef.h"
#include "Util.h"
#include "StaticData.h"
#include "Manager.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
HypothesisStackCubePruning::HypothesisStackCubePruning(Manager& manager) :
  HypothesisStack(manager)
{
  m_nBestIsEnabled = manager.options()->nbest.enabled;
  m_bestScore = -std::numeric_limits<float>::infinity();
  m_worstScore = -std::numeric_limits<float>::infinity();
  m_deterministic = manager.options()->cube.deterministic_search;
}

/** remove all hypotheses from the collection */
void HypothesisStackCubePruning::RemoveAll()
{
  // delete all bitmap accessors;
  _BMType::iterator iter;
  for (iter = m_bitmapAccessor.begin(); iter != m_bitmapAccessor.end(); ++iter) {
    delete iter->second;
  }
}

pair<HypothesisStackCubePruning::iterator, bool> HypothesisStackCubePruning::Add(Hypothesis *hypo)
{
  std::pair<iterator, bool> ret = m_hypos.insert(hypo);

  if (ret.second) {
    // equiv hypo doesn't exists
    VERBOSE(3,"added hyp to stack");

    // Update best score, if this hypothesis is new best
    if (hypo->GetFutureScore() > m_bestScore) {
      VERBOSE(3,", best on stack");
      m_bestScore = hypo->GetFutureScore();
      // this may also affect the worst score
      if ( m_bestScore + m_beamWidth > m_worstScore )
        m_worstScore = m_bestScore + m_beamWidth;
    }

    // Prune only if stack is twice as big as needed (lazy pruning)
    VERBOSE(3,", now size " << m_hypos.size());
    if (m_hypos.size() > 2*m_maxHypoStackSize-1) {
      PruneToSize(m_maxHypoStackSize);
    } else {
      VERBOSE(3,std::endl);
    }
  }

  return ret;
}

bool HypothesisStackCubePruning::AddPrune(Hypothesis *hypo)
{
  if (hypo->GetFutureScore() == - std::numeric_limits<float>::infinity()) {
    m_manager.GetSentenceStats().AddDiscarded();
    VERBOSE(3,"discarded, constraint" << std::endl);
    delete hypo;
    return false;
  }

  if (hypo->GetFutureScore() < m_worstScore) {
    // too bad for stack. don't bother adding hypo into collection
    m_manager.GetSentenceStats().AddDiscarded();
    VERBOSE(3,"discarded, too bad for stack" << std::endl);
    delete hypo;
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
  assert(iterExisting != m_hypos.end());
  Hypothesis *hypoExisting = *iterExisting;

  m_manager.GetSentenceStats().AddRecombination(*hypo, **iterExisting);

  // found existing hypo with same target ending.
  // keep the best 1
  if (hypo->GetFutureScore() > hypoExisting->GetFutureScore()) {
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
      UTIL_THROW(util::Exception, "Should have added hypothesis " << **iterExisting);
    }
    return false;
  } else {
    // already storing the best hypo. discard current hypo
    VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
    if (m_nBestIsEnabled) {
      hypoExisting->AddArc(hypo);
    } else {
      delete hypo;
    }
    return false;
  }
}

void HypothesisStackCubePruning::AddInitial(Hypothesis *hypo)
{
  std::pair<iterator, bool> addRet = Add(hypo);
  UTIL_THROW_IF2(!addRet.second,
                 "Should have added hypothesis " << *hypo);

  const Bitmap &bitmap = hypo->GetWordsBitmap();
  AddBitmapContainer(bitmap, *this);
}

void HypothesisStackCubePruning::PruneToSize(size_t newSize)
{
  if ( newSize == 0) return; // no limit

  if (m_hypos.size() > newSize) { // ok, if not over the limit
    priority_queue<float> bestScores;

    // push all scores to a heap
    // (but never push scores below m_bestScore+m_beamWidth)
    iterator iter = m_hypos.begin();
    float score = 0;
    while (iter != m_hypos.end()) {
      Hypothesis *hypo = *iter;
      score = hypo->GetFutureScore();
      if (score > m_bestScore+m_beamWidth) {
        bestScores.push(score);
      }
      ++iter;
    }

    // pop the top newSize scores (and ignore them, these are the scores of hyps that will remain)
    //  ensure to never pop beyond heap size
    size_t minNewSizeHeapSize = newSize > bestScores.size() ? bestScores.size() : newSize;
    for (size_t i = 1 ; i < minNewSizeHeapSize ; i++)
      bestScores.pop();

    // and remember the threshold
    float scoreThreshold = bestScores.top();

    // delete all hypos under score threshold
    iter = m_hypos.begin();
    while (iter != m_hypos.end()) {
      Hypothesis *hypo = *iter;
      float score = hypo->GetFutureScore();
      if (score < scoreThreshold) {
        iterator iterRemove = iter++;
        Remove(iterRemove);
        m_manager.GetSentenceStats().AddPruning();
      } else {
        ++iter;
      }
    }
    VERBOSE(3,", pruned to size " << size() << endl);

    IFVERBOSE(3) {
      TRACE_ERR("stack now contains: ");
      for(iter = m_hypos.begin(); iter != m_hypos.end(); iter++) {
        Hypothesis *hypo = *iter;
        TRACE_ERR( hypo->GetId() << " (" << hypo->GetFutureScore() << ") ");
      }
      TRACE_ERR( endl);
    }

    // set the worstScore, so that newly generated hypotheses will not be added if worse than the worst in the stack
    m_worstScore = scoreThreshold;
  }
}

const Hypothesis *HypothesisStackCubePruning::GetBestHypothesis() const
{
  if (!m_hypos.empty()) {
    const_iterator iter = m_hypos.begin();
    Hypothesis *bestHypo = *iter;
    while (++iter != m_hypos.end()) {
      Hypothesis *hypo = *iter;
      if (hypo->GetFutureScore() > bestHypo->GetFutureScore())
        bestHypo = hypo;
    }
    return bestHypo;
  }
  return NULL;
}

vector<const Hypothesis*> HypothesisStackCubePruning::GetSortedList() const
{
  vector<const Hypothesis*> ret;
  ret.reserve(m_hypos.size());
  std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
  sort(ret.begin(), ret.end(), CompareHypothesisTotalScore());

  return ret;
}


void HypothesisStackCubePruning::CleanupArcList()
{
  // only necessary if n-best calculations are enabled
  if (!m_nBestIsEnabled) return;

  iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    Hypothesis *mainHypo = *iter;
    mainHypo->CleanupArcList(this->m_manager.options()->nbest.nbest_size, this->m_manager.options()->NBestDistinct());
  }
}

void HypothesisStackCubePruning::SetBitmapAccessor(const Bitmap &newBitmap
    , HypothesisStackCubePruning &stack
    , const Range &/*range*/
    , BitmapContainer &bitmapContainer
    , const SquareMatrix &estimatedScores
    , const TranslationOptionList &transOptList)
{
  BitmapContainer *bmContainer =   AddBitmapContainer(newBitmap, stack);
  BackwardsEdge *edge = new BackwardsEdge(bitmapContainer
                                          , *bmContainer
                                          , transOptList
                                          , estimatedScores
                                          , m_manager.GetSource()
                                          , m_deterministic);
  bmContainer->AddBackwardsEdge(edge);
}


TO_STRING_BODY(HypothesisStackCubePruning);


// friend
std::ostream& operator<<(std::ostream& out, const HypothesisStackCubePruning& hypoColl)
{
  HypothesisStackCubePruning::const_iterator iter;

  for (iter = hypoColl.begin() ; iter != hypoColl.end() ; ++iter) {
    const Hypothesis &hypo = **iter;
    out << hypo << endl;

  }
  return out;
}

void
HypothesisStackCubePruning::AddHypothesesToBitmapContainers()
{
  HypothesisStackCubePruning::const_iterator iter;
  for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter) {
    Hypothesis *h = *iter;
    const Bitmap &bitmap = h->GetWordsBitmap();
    BitmapContainer *container = m_bitmapAccessor[&bitmap];
    container->AddHypothesis(h);
  }
}

BitmapContainer *HypothesisStackCubePruning::AddBitmapContainer(const Bitmap &bitmap, HypothesisStackCubePruning &stack)
{
  _BMType::iterator iter = m_bitmapAccessor.find(&bitmap);

  BitmapContainer *bmContainer;
  if (iter == m_bitmapAccessor.end()) {
    bmContainer = new BitmapContainer(bitmap, stack, m_deterministic);
    m_bitmapAccessor[&bitmap] = bmContainer;
  } else {
    bmContainer = iter->second;
  }

  return bmContainer;
}

}

