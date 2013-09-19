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
  //std::cout << "new RuleCubeMBOT()" << std::endl;
  //std::cout << "Making rule cube item MBOT" << std::endl;

  /*std::cout << "RCMBOT : Check Target Phrase Collection : " << std::endl;

    TargetPhraseCollection tpc = m_mbotTransOpt.GetTargetPhraseCollection();
    TargetPhrase * tp = *(tpc.begin());
    //cast to target phrase MBOT
    TargetPhraseMBOT * targetPhrase = static_cast<TargetPhraseMBOT*>(tp);
    std::cout << "RCMBOT : Adress : " << &targetPhrase << *targetPhrase << std::endl;*/

  RuleCubeItemMBOT *item = new RuleCubeItemMBOT(transOpt, allChartCells);
  //std::cout << "Inserting item..." << std::endl;
  m_mbotCovered.insert(item);
  //std::cout << "Rule Cube Inserted into m_covered" << std::endl;
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->EstimateScore();
  } else {
     //std::cerr << "RULE CUBE : Creating Hypothesis" << std::endl;

	//only create hypothesis if there we have more target phrases with source lhs matching the cell label
	if(item->GetTranslationDimensionMBOT().HasMoreMatchingTargetPhrase())
	{
		//increment the translation dimension until a target phrase matching the label of the chart cell is found
		while(item->GetTranslationDimensionMBOT().GetPosition() != item->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase())
		{
			item->IncrementTranslationDimension();
			//std::cerr << "CURRENT POSITION "<< item->GetTranslationDimensionMBOT().GetPosition() << std::endl;
			//std::cerr << "POSITION OF TARGET " << item->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase() << std::endl;
		}
		//std::cerr << "WE HAVE A MATCHING TARGET PHRASE AT POSITION : " << item->GetTranslationDimensionMBOT().GetPosition() << std::endl;
		item->CreateHypothesis(transOpt, manager);
	}
	/*else
	{
		std::cerr << "NO MORE MATCHING TARGET PHRASE : NULL HYPO CREATED... " << std::endl;
	}*/
  }
  m_mbotQueue.push(item);
  //std::cout << "RCMBOT : Hypo pushed into queue" << std::endl;
}

RuleCubeMBOT::~RuleCubeMBOT()
{
  RemoveAllInColl(m_mbotCovered);
}

RuleCubeItemMBOT *RuleCubeMBOT::PopMBOT(ChartManager &manager)
{
  //std::cout << "POPPING RULE CUBE" << std::endl;
  RuleCubeItemMBOT *item = m_mbotQueue.top();
  //std::cout << "FOUND TOP"<< std::endl;
  m_mbotQueue.pop();
  //std::cout << "POPPED QUEUE"<< std::endl;
  CreateNeighborsMBOT(*item, manager);
  //std::cout << "Neighnord done"<< std::endl;
  return item;
}

// create new RuleCube for neighboring principle rules
void RuleCubeMBOT::CreateNeighborsMBOT(const RuleCubeItemMBOT &item, ChartManager &manager)
{
  //std::cerr << "CREATING MBOT NEIGHBORS... "<< std::endl;
  // create neighbor along translation dimension
  const TranslationDimension &translationDimension =
    item.GetTranslationDimensionMBOT();
  //only create neighbor on tranlsation dimension if we have more matching target phrases
  if (translationDimension.HasMoreMatchingTargetPhrase()) {

	  //std::cerr << "CREATING NEIGHBOR FOR TRANSLATION AND HYPOTHESIS DIMENSIONS... " << std::endl;
	  CreateNeighborMBOT(item, -1, manager);

	  // create neighbors along all hypothesis dimensions
	   for (size_t i = 0; i < item.GetHypothesisDimensionsMBOT().size(); ++i) {
	        //std::cout << "creating neighbors along translation dimensions" << std::endl;
	     const HypothesisDimensionMBOT &dimension = item.GetHypothesisDimensionsMBOT()[i];
	     if (dimension.HasMoreHypo()) {
	       //std::cerr << "CREATING NEIGHBOR FOR HYPOTHESIS DIMENSION... " << std::endl;
	       CreateNeighborMBOT(item, i, manager);
	     }
	   }

  }
  /*else
  {
	  std::cerr << "NO MORE MATCHING TARGET PHRASE : NO NEIGHBOR CREATED... " << std::endl;
  }*/

  //std::cout << "Out of neighbor creation " << std::endl;
}

void RuleCubeMBOT::CreateNeighborMBOT(const RuleCubeItemMBOT &item, int dimensionIndex,
                              ChartManager &manager)
{
  //std::cout << "Making new rule cube item" << std::endl;
  RuleCubeItemMBOT *newItem = new RuleCubeItemMBOT(item, dimensionIndex);
  //std::cerr << "RC : CREATED COPY OF RULE CUBE ITEM" << std::endl;
  //std::cerr << "POSITION OF NEW ITEM : " << newItem->GetTranslationDimensionMBOT().GetPosition() << std::endl;
  //std::cout << "Inserting new item" << std::endl;
  std::pair<ItemSetMBOT::iterator, bool> result = m_mbotCovered.insert(newItem);
  if (!result.second) {
    //std::cerr << "ITEM IS ALREADY COVERED..." << std::endl;
    delete newItem;  // already seen it
  } else {
    if (StaticData::Instance().GetCubePruningLazyScoring()) {
      //std::cout << "estimating score" << std::endl;
      newItem->EstimateScore();
    } else {
      //std::cout << "Creating new hypothesis" << std::endl;
      //std::cerr << "POSITION OF NEW ITEM BEFORE CREATING HYPO : " << newItem->GetTranslationDimensionMBOT().GetPosition() << std::endl;
      //only create hypothesis if there we have more target phrases with source lhs matching the cell label
      	if(newItem->GetTranslationDimensionMBOT().HasMoreMatchingTargetPhrase())
      	{
      		//std::cerr << "POSITION OF MATCHING TARGET PHRASE : " << newItem->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase() << std::endl;
      		//increment the translation dimension until a target phrase matching the label of the chart cell is found
      		while(newItem->GetTranslationDimensionMBOT().GetPosition() != newItem->GetTranslationDimensionMBOT().GetPositionOfMatchingTargetPhrase())
      		{
      			newItem->IncrementTranslationDimension();
      			//std::cerr << "POSITION OF NEW ITEM AFTER INCREMENTING DIMENSION : " << newItem->GetTranslationDimensionMBOT().GetPosition() << std::endl;

      		}
      		//std::cerr << "WE HAVE A MATCHING TARGET PHRASE AT POSITION : " << newItem->GetTranslationDimensionMBOT().GetPosition() << std::endl;
      		newItem->CreateHypothesis(m_mbotTransOpt, manager);
      	}
      	/*else
      	{
      		std::cerr << "NO MORE MATCHING TARGET PHRASE : NULL HYPO CREATED... " << std::endl;
      	}*/
    }
    m_mbotQueue.push(newItem);
  }
}

}

