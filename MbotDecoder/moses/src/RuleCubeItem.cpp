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
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionMBOT.h"
#include "ChartTranslationOptionCollection.h"
#include "DotChart.h"
#include "RuleCubeItem.h"
#include "RuleCubeItemMBOT.h"
#include "RuleCubeQueue.h"
#include "WordsRange.h"
#include "Util.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

std::size_t hash_value(const HypothesisDimension &dimension)
{
  boost::hash<const ChartHypothesis*> hasher;
  return hasher(dimension.GetHypothesis());
}

RuleCubeItem::RuleCubeItem(const ChartTranslationOption &transOpt,
                           const ChartCellCollection &allChartCells)
  : m_translationDimension(0,transOpt.GetTargetPhraseCollection().GetCollection())
  , m_hypothesis(0)
{
   //FB: BEWARE : NO CONSTRUCTION OF HYPOTHESIS DIMENSION IN RULE CUBE ITEM
   //std::cout << "NO CONSTRUCTION OF NON MBOT RULE CUBE ITEM"<< std::endl;
  //CreateHypothesisDimensions(transOpt.GetDottedRule(), allChartCells);
}

// create the RuleCube from an existing one, differing only in one dimension
RuleCubeItem::RuleCubeItem(const RuleCubeItem &copy, int hypoDimensionIncr)
  : m_translationDimension(copy.m_translationDimension)
  , m_hypothesisDimensions(copy.m_hypothesisDimensions)
  , m_hypothesis(0)
{
   //FB : BEWARE : NO COPY CONSTRUCTION FOR RULE CUBE ITEM
  /*if (hypoDimensionIncr == -1) {
    m_translationDimension.IncrementPos();
  } else {
    HypothesisDimension &dimension = m_hypothesisDimensions[hypoDimensionIncr];
    dimension.IncrementPos();
  }*/
}

RuleCubeItem::~RuleCubeItem()
{
  delete m_hypothesis;
}

ChartHypothesis *RuleCubeItem::ReleaseHypothesis()
{
  CHECK(m_hypothesis);
  ChartHypothesis *hypo = m_hypothesis;
  m_hypothesis = 0;
  return hypo;
}

void RuleCubeItem::EstimateScore()
{
  m_score = m_translationDimension.GetTargetPhrase()->GetFutureScore();
  std::vector<HypothesisDimension>::const_iterator p;
  for (p = m_hypothesisDimensions.begin();
       p != m_hypothesisDimensions.end(); ++p) {
    m_score += p->GetHypothesis()->GetTotalScore();
  }
}

void RuleCubeItem::CreateHypothesis(const ChartTranslationOption &transOpt,
                                    ChartManager &manager)
{
  m_hypothesis = new ChartHypothesis(transOpt, *this, manager);
  m_hypothesis->CalcScore();
  m_score = m_hypothesis->GetTotalScore();
}

// for each non-terminal, create a ordered list of matching hypothesis from the
// chart
void RuleCubeItem::CreateHypothesisDimensions(
  const DottedRule &dottedRule,
  const ChartCellCollection &allChartCells)
{
  CHECK(!dottedRule.IsRoot());

  const DottedRule *prev = dottedRule.GetPrev();
  if (!prev->IsRoot()) {
    CreateHypothesisDimensions(*prev, allChartCells);
  }

  // only deal with non-terminals
  if (dottedRule.IsNonTerminal()) {
    // get a sorted list of the underlying hypotheses
    const ChartCellLabel &cellLabel = dottedRule.GetChartCellLabel();
    const ChartHypothesisCollection *hypoColl = cellLabel.GetStack();
    CHECK(hypoColl);
    const HypoList &hypoList = hypoColl->GetSortedHypotheses();

    // there have to be hypothesis with the desired non-terminal
    // (otherwise the rule would not be considered)
    CHECK(!hypoList.empty());

    // create a list of hypotheses that match the non-terminal
    HypothesisDimension dimension(0, hypoList);
    // add them to the vector for such lists
    m_hypothesisDimensions.push_back(dimension);
  }
}

bool RuleCubeItem::operator<(const RuleCubeItem &compare) const
{
  if (1==1)
    return true;
  if (m_translationDimension == compare.m_translationDimension) {
    return m_hypothesisDimensions < compare.m_hypothesisDimensions;
  }
  return m_translationDimension < compare.m_translationDimension;
}

}
