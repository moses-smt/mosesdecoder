/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "ChartCell.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptions.h"
#include "ChartManager.h"
#include "RuleCubeItem.h"
#include "RuleCubeQueue.h"
#include "Range.h"
#include "Util.h"
#include "util/exception.hh"

#include <boost/functional/hash.hpp>

namespace Moses
{

std::size_t hash_value(const HypothesisDimension &dimension)
{
  boost::hash<const ChartHypothesis*> hasher;
  return hasher(dimension.GetHypothesis());
}

RuleCubeItem::RuleCubeItem(const ChartTranslationOptions &transOpt,
                           const ChartCellCollection &/*allChartCells*/)
  : m_translationDimension(0, transOpt.GetTargetPhrases())
  , m_hypothesis(0)
{
  CreateHypothesisDimensions(transOpt.GetStackVec());
}

// create the RuleCube from an existing one, differing only in one dimension
RuleCubeItem::RuleCubeItem(const RuleCubeItem &copy, int hypoDimensionIncr)
  : m_translationDimension(copy.m_translationDimension)
  , m_hypothesisDimensions(copy.m_hypothesisDimensions)
  , m_hypothesis(0)
{
  if (hypoDimensionIncr == -1) {
    m_translationDimension.IncrementPos();
  } else {
    HypothesisDimension &dimension = m_hypothesisDimensions[hypoDimensionIncr];
    dimension.IncrementPos();
  }
}

RuleCubeItem::~RuleCubeItem()
{
  delete m_hypothesis;
}

void RuleCubeItem::EstimateScore()
{
  m_score = m_translationDimension.GetTranslationOption()->GetPhrase().GetFutureScore();
  std::vector<HypothesisDimension>::const_iterator p;
  for (p = m_hypothesisDimensions.begin();
       p != m_hypothesisDimensions.end(); ++p) {
    m_score += p->GetHypothesis()->GetFutureScore();
  }
}

void RuleCubeItem::CreateHypothesis(const ChartTranslationOptions &transOpt,
                                    ChartManager &manager)
{
  m_hypothesis = new ChartHypothesis(transOpt, *this, manager);
  m_hypothesis->EvaluateWhenApplied();
  m_score = m_hypothesis->GetFutureScore();
}

ChartHypothesis *RuleCubeItem::ReleaseHypothesis()
{
  UTIL_THROW_IF2(m_hypothesis == NULL, "Hypothesis is NULL");
  ChartHypothesis *hypo = m_hypothesis;
  m_hypothesis = NULL;
  return hypo;
}

// for each non-terminal, create a ordered list of matching hypothesis from the
// chart
void RuleCubeItem::CreateHypothesisDimensions(const StackVec &stackVec)
{
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end();
       ++p) {
    const HypoList *stack = (*p)->GetStack().cube;
    assert(stack);

    // there have to be hypothesis with the desired non-terminal
    // (otherwise the rule would not be considered)
    assert(!stack->empty());

    // create a list of hypotheses that match the non-terminal
    HypothesisDimension dimension(0, *stack);
    // add them to the vector for such lists
    m_hypothesisDimensions.push_back(dimension);
  }
}

bool RuleCubeItem::operator<(const RuleCubeItem &compare) const
{
  if (m_translationDimension == compare.m_translationDimension) {
    return m_hypothesisDimensions < compare.m_hypothesisDimensions;
  }
  return m_translationDimension < compare.m_translationDimension;
}

}
