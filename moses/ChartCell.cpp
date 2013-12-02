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
#include "ChartCell.h"
#include "ChartCellCollection.h"
#include "RuleCubeQueue.h"
#include "RuleCube.h"
#include "WordsRange.h"
#include "Util.h"
#include "StaticData.h"
#include "ChartTranslationOptions.h"
#include "ChartTranslationOptionList.h"
#include "ChartManager.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
extern bool g_debug;

ChartCellBase::ChartCellBase(size_t startPos, size_t endPos) :
  m_coverage(startPos, endPos),
  m_targetLabelSet(m_coverage) {}

ChartCellBase::~ChartCellBase() {}

/** Constructor
 * \param startPos endPos range of this cell
 * \param manager pointer back to the manager
 */
ChartCell::ChartCell(size_t startPos, size_t endPos, ChartManager &manager) :
  ChartCellBase(startPos, endPos), m_manager(manager)
{
  const StaticData &staticData = StaticData::Instance();
  m_nBestIsEnabled = staticData.IsNBestEnabled();
}

ChartCell::~ChartCell() {}

/** Add the given hypothesis to the cell.
 *  Returns true if added, false if not. Maybe it already exists in the collection or score falls below threshold etc.
 *  This function just calls the correspondind AddHypothesis() in ChartHypothesisCollection
 *  \param hypo Hypothesis to be added
 */
bool ChartCell::AddHypothesis(ChartHypothesis *hypo)
{
  const Word &targetLHS = hypo->GetTargetLHS();
  return m_hypoColl[targetLHS].AddHypothesis(hypo, m_manager);
}

/** Prune each collection in this cell to a particular size */
void ChartCell::PruneToSize()
{
  MapType::iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    ChartHypothesisCollection &coll = iter->second;
    coll.PruneToSize(m_manager);
  }
}

/** Decoding at span level: fill chart cell with hypotheses
 *  (implementation of cube pruning)
 * \param transOptList list of applicable rules to create hypotheses for the cell
 * \param allChartCells entire chart - needed to look up underlying hypotheses
 */
void ChartCell::ProcessSentence(const ChartTranslationOptionList &transOptList
                                , const ChartCellCollection &allChartCells)
{
  const StaticData &staticData = StaticData::Instance();

  // priority queue for applicable rules with selected hypotheses
  RuleCubeQueue queue(m_manager);

  // add all trans opt into queue. using only 1st child node.
  for (size_t i = 0; i < transOptList.GetSize(); ++i) {
    const ChartTranslationOptions &transOpt = transOptList.Get(i);
    RuleCube *ruleCube = new RuleCube(transOpt, allChartCells, m_manager);
    queue.Add(ruleCube);
  }

  // pluck things out of queue and add to hypo collection
  const size_t popLimit = staticData.GetCubePruningPopLimit();
  for (size_t numPops = 0; numPops < popLimit && !queue.IsEmpty(); ++numPops) {
    ChartHypothesis *hypo = queue.Pop();
    AddHypothesis(hypo);
  }
}

//! call SortHypotheses() in each hypo collection in this cell
void ChartCell::SortHypotheses()
{
  UTIL_THROW_IF2(!m_targetLabelSet.Empty(), "Already sorted");

  MapType::iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    ChartHypothesisCollection &coll = iter->second;
    coll.SortHypotheses();
    m_targetLabelSet.AddConstituent(iter->first, &coll.GetSortedHypotheses());
  }
}

/** Return the highest scoring hypothesis out of all the  hypo collection in this cell */
const ChartHypothesis *ChartCell::GetBestHypothesis() const
{
  const ChartHypothesis *ret = NULL;
  float bestScore = -std::numeric_limits<float>::infinity();

  MapType::const_iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    const HypoList &sortedList = iter->second.GetSortedHypotheses();
    if (sortedList.size() > 0) {
      const ChartHypothesis *hypo = sortedList[0];
      if (hypo->GetTotalScore() > bestScore) {
        bestScore = hypo->GetTotalScore();
        ret = hypo;
      }
    }
  }

  return ret;
}

//! call CleanupArcList() in each hypo collection in this cell
void ChartCell::CleanupArcList()
{
  // only necessary if n-best calculations are enabled
  if (!m_nBestIsEnabled) return;

  MapType::iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    ChartHypothesisCollection &coll = iter->second;
    coll.CleanupArcList();
  }
}

//! debug info - size of each hypo collection in this cell
void ChartCell::OutputSizes(std::ostream &out) const
{
  MapType::const_iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    const Word &targetLHS = iter->first;
    const ChartHypothesisCollection &coll = iter->second;

    out << targetLHS << "=" << coll.GetSize() << " ";
  }
}

//! debug info - total number of hypos in all hypo collection in this cell
size_t ChartCell::GetSize() const
{
  size_t ret = 0;
  MapType::const_iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    const ChartHypothesisCollection &coll = iter->second;

    ret += coll.GetSize();
  }

  return ret;
}

const HypoList *ChartCell::GetAllSortedHypotheses() const
{
  HypoList *ret = new HypoList();

  MapType::const_iterator iter;
  for (iter = m_hypoColl.begin(); iter != m_hypoColl.end(); ++iter) {
    const ChartHypothesisCollection &coll = iter->second;
    const HypoList &list = coll.GetSortedHypotheses();
    std::copy(list.begin(), list.end(), std::inserter(*ret, ret->end()));
  }
  return ret;
}

//! call GetSearchGraph() for each hypo collection
void ChartCell::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const std::map<unsigned, bool> &reachable) const
{
  MapType::const_iterator iterOutside;
  for (iterOutside = m_hypoColl.begin(); iterOutside != m_hypoColl.end(); ++iterOutside) {
    const ChartHypothesisCollection &coll = iterOutside->second;
    coll.GetSearchGraph(translationId, outputSearchGraphStream, reachable);
  }
}

std::ostream& operator<<(std::ostream &out, const ChartCell &cell)
{
  ChartCell::MapType::const_iterator iterOutside;
  for (iterOutside = cell.m_hypoColl.begin(); iterOutside != cell.m_hypoColl.end(); ++iterOutside) {
    const Word &targetLHS = iterOutside->first;
    cerr << targetLHS << ":" << endl;

    const ChartHypothesisCollection &coll = iterOutside->second;
    cerr << coll;
  }

  /*
  ChartCell::HCType::const_iterator iter;
  for (iter = cell.m_hypos.begin(); iter != cell.m_hypos.end(); ++iter)
  {
  	const ChartHypothesis &hypo = **iter;
  	out << hypo << endl;
  }
   */

  return out;
}

} // namespace
