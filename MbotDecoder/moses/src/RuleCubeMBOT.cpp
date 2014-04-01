// $Id: RuleCubeMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#include "ChartCellMBOT.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptionMBOT.h"
#include "ChartTranslationOptionCollection.h"
#include "RuleCubeMBOT.h"
#include "RuleCubeQueueMBOT.h"
#include "StaticData.h"
#include "Util.h"
#include "WordsRange.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

// initialise the RuleCube by creating the top-left corner item
RuleCubeMBOT::RuleCubeMBOT(const ChartTranslationOptionMBOT &transOpt,
                   const ChartCellCollection &allChartCells,
                   ChartManager &manager)
                   : RuleCube(transOpt,allChartCells,manager)
  , m_mbotTransOpt(transOpt)
{
  RuleCubeItemMBOT *item = new RuleCubeItemMBOT(transOpt, allChartCells);
  m_mbotCovered.insert(item);
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->EstimateScore();
  } else {
	//only create hypothesis if there we have more target phrases with source lhs matching the cell label
	if(item->GetTranslationDimensionMBOT().HasMoreMatchingTargetPhrase())
	{
		//increment the translation dimension until a target phrase matching the label of the chart cell is found
		while(item->GetTranslationDimensionMBOT().GetPosition() != item->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase())
		{
			item->IncrementTranslationDimension();
		}
		item->CreateHypothesis(transOpt, manager);
	}
  }
  m_mbotQueue.push(item);
}

RuleCubeMBOT::~RuleCubeMBOT()
{
  RemoveAllInColl(m_mbotCovered);
}

RuleCubeItemMBOT *RuleCubeMBOT::PopMBOT(ChartManager &manager)
{
  RuleCubeItemMBOT *item = m_mbotQueue.top();
  m_mbotQueue.pop();
  CreateNeighborsMBOT(*item, manager);
  return item;
}

// create new RuleCube for neighboring principle rules
void RuleCubeMBOT::CreateNeighborsMBOT(const RuleCubeItemMBOT &item, ChartManager &manager)
{
  // create neighbor along translation dimension
  const TranslationDimension &translationDimension =
    item.GetTranslationDimensionMBOT();
  //only create neighbor on tranlsation dimension if we have more matching target phrases
  if (translationDimension.HasMoreMatchingTargetPhrase()) {
	  CreateNeighborMBOT(item, -1, manager);

	  // create neighbors along all hypothesis dimensions
	   for (size_t i = 0; i < item.GetHypothesisDimensionsMBOT().size(); ++i) {
	     const HypothesisDimensionMBOT &dimension = item.GetHypothesisDimensionsMBOT()[i];
	     if (dimension.HasMoreHypo()) {
	       CreateNeighborMBOT(item, i, manager);
	     }
	   }

  }
}

void RuleCubeMBOT::CreateNeighborMBOT(const RuleCubeItemMBOT &item, int dimensionIndex,
                              ChartManager &manager)
{
  RuleCubeItemMBOT *newItem = new RuleCubeItemMBOT(item, dimensionIndex);
  std::pair<ItemSetMBOT::iterator, bool> result = m_mbotCovered.insert(newItem);
  if (!result.second) {
    delete newItem;  // already seen it
  } else {
    if (StaticData::Instance().GetCubePruningLazyScoring()) {
      newItem->EstimateScore();
    } else {
      	if(newItem->GetTranslationDimensionMBOT().HasMoreMatchingTargetPhrase())
      	{
      		//increment the translation dimension until a target phrase matching the label of the chart cell is found
      		while(newItem->GetTranslationDimensionMBOT().GetPosition() != newItem->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase())
      		{
      			newItem->IncrementTranslationDimension();
      		}
      		newItem->CreateHypothesis(m_mbotTransOpt, manager);
      	}
    }
    m_mbotQueue.push(newItem);
  }
}

}

