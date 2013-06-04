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

#include "ChartCell.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptions.h"
#include "RuleCube.h"
#include "RuleCubeQueue.h"
#include "StaticData.h"
#include "Util.h"
#include "WordsRange.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

// initialise the RuleCube by creating the top-left corner item
RuleCube::RuleCube(const ChartTranslationOptions &transOpt,
                   const ChartCellCollection &allChartCells,
                   ChartManager &manager)
  : m_transOpt(transOpt)
{
  RuleCubeItem *item = new RuleCubeItem(transOpt, allChartCells);
  m_covered.insert(item);
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->EstimateScore();
  } else {
    item->CreateHypothesis(transOpt, manager);
  }
  m_queue.push(item);
}

RuleCube::~RuleCube()
{
  RemoveAllInColl(m_covered);
}

RuleCubeItem *RuleCube::Pop(ChartManager &manager)
{
  RuleCubeItem *item = m_queue.top();
  m_queue.pop();
  CreateNeighbors(*item, manager);
  return item;
}

// create new RuleCube for neighboring principle rules
void RuleCube::CreateNeighbors(const RuleCubeItem &item, ChartManager &manager)
{
  // create neighbor along translation dimension
  const TranslationDimension &translationDimension =
    item.GetTranslationDimension();
  if (translationDimension.HasMoreTranslations()) {
    CreateNeighbor(item, -1, manager);
  }

  // create neighbors along all hypothesis dimensions
  for (size_t i = 0; i < item.GetHypothesisDimensions().size(); ++i) {
    const HypothesisDimension &dimension = item.GetHypothesisDimensions()[i];
    if (dimension.HasMoreHypo()) {
      CreateNeighbor(item, i, manager);
    }
  }
}

void RuleCube::CreateNeighbor(const RuleCubeItem &item, int dimensionIndex,
                              ChartManager &manager)
{
  RuleCubeItem *newItem = new RuleCubeItem(item, dimensionIndex);
  std::pair<ItemSet::iterator, bool> result = m_covered.insert(newItem);
  if (!result.second) {
    delete newItem;  // already seen it
  } else {
    if (StaticData::Instance().GetCubePruningLazyScoring()) {
      newItem->EstimateScore();
    } else {
      newItem->CreateHypothesis(m_transOpt, manager);
    }
    m_queue.push(newItem);
  }
}

}
