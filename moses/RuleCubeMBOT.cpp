//Fabienne Braune
//RuleCube conaining l-MBOT RuleCubeItems : Would be cool to inherit from RuleCube but then how can I cast ChartCellCollectionMBOT
//to ChartCellCollection when instantiating base class RuleCube in constructor.

#include "ChartCellMBOT.h"
#include "ChartCellCollection.h"
#include "RuleCubeMBOT.h"
#include "RuleCube.h"
#include "StaticData.h"
#include "Util.h"
#include "WordsRange.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

// initialise the RuleCube by creating the top-left corner item
RuleCubeMBOT::RuleCubeMBOT(const ChartTranslationOptions &transOpt,
                   const ChartCellCollection& allChartCells,
                   ChartManager &manager)
   : RuleCube(transOpt,allChartCells,manager)
  , m_mbotTransOpt(transOpt)
{
  RuleCubeItemMBOT *item = new RuleCubeItemMBOT(transOpt, allChartCells);
  m_mbotCovered.insert(item);
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->EstimateScore();
  } else {

	//Fabienne Braune : check matching option
	if(StaticData::Instance().IsMatchingSourceAtRuleApplication() == 1)
	{
		//Fabienne Braune : if the matching option is on, then only create hypothesis for matching target phrase
		if(item->GetTranslationDimension().HasMoreMatchingTargetPhrase())
		{
			//increment the translation dimension until a target phrase matching the label of the chart cell is found
			while(item->GetTranslationDimension().GetPosition() != item->GetTranslationDimension().GetPositionOfMatchingTargetPhrase())
			{
				item->IncrementTranslationDimension();
			}
			item->CreateHypothesis(transOpt, manager);
		}
	}
	else //Fabienne Braune : If we don't match the input target phrase then always create hypothesis
	{
		item->CreateHypothesis(transOpt, manager);
	}
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

  RuleCubeItemMBOT *item = m_mbotQueue.top();
  //std::cout << "FOUND TOP"<< std::endl;
  m_mbotQueue.pop();
  std::cerr << "POPPED QUEUE, CREATE NEIGHBORS..."<< std::endl;
  CreateNeighborsMBOT(item, manager);
  //std::cout << "Neighnord done"<< std::endl;
  return item;
}

// create new RuleCube for neighboring principle rules
//Fabienne Braune : Two ways to create neighbors : standard and with check if the source phrase matches the input parse tree.
//TODO : The check could be moved out of here in method PopMBOT
void RuleCubeMBOT::CreateNeighborsMBOT(const RuleCubeItemMBOT* item, ChartManager &manager)
{
  // create neighbor along translation dimension
  const TranslationDimension &translationDimension =
    item->GetTranslationDimension();

  //Fabienne Braune : check matching option
  if(StaticData::Instance().IsMatchingSourceAtRuleApplication() == 1)
  {
	  //Fabienne Braune : if matching option is on, only create neighbor on translation dimension if we have more matching target phrases
	  if (translationDimension.HasMoreMatchingTargetPhrase()) {

		  //std::cerr << "CREATING NEIGHBOR FOR TRANSLATION AND HYPOTHESIS DIMENSIONS... " << std::endl;
		  CreateMatchingNeighborMBOT(item, -1, manager);

	  	// create neighbors along all hypothesis dimensions
	  		for (size_t i = 0; i < item->GetHypothesisDimensionsMBOT().size(); ++i) {
	  			const HypothesisDimension &dimension = item->GetHypothesisDimensionsMBOT()[i];
	  			if (dimension.HasMoreHypo()) {
	  				CreateMatchingNeighborMBOT(item, i, manager);
	  			}
	  		}
	  }
  }
  else //Fabienne Braune : if matching option is off create neighbors for all target phrases
  {
	  std::cerr << "HERE WE DO NOT MATCH..." << std::endl;

	  // create neighbor along translation dimension
	  if (translationDimension.HasMoreTranslations()) {
		  std::cerr << "Creating Translation neighbor (target phrase)..." << std::endl;
		  CreateNeighborMBOT(item, -1, manager);
	  }

	  // create neighbors along all hypothesis dimensions
	  for (size_t i = 0; i < item->GetHypothesisDimensionsMBOT().size(); ++i) {
		  const HypothesisDimension &dimension = item->GetHypothesisDimensions()[i];
		  if (dimension.HasMoreHypo()) {
			  std::cerr << "Creating Hypothesis neighbor (non terminal)..." << std::endl;
			  CreateNeighborMBOT(item, i, manager);
		  }
	  }
  }
}

void RuleCubeMBOT::CreateNeighborMBOT(const RuleCubeItemMBOT* item, int dimensionIndex,
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
      newItem->CreateHypothesis(m_mbotTransOpt, manager);
    }
    m_mbotQueue.push(newItem);
  }
}

//Fabienne : Create neighbors with target phrases with source sides that match the input parse Tree
//This is very hacky : we iterate over the translation dimension until we find a target phrase having a source phrase that matches
//the parse tree.
void RuleCubeMBOT::CreateMatchingNeighborMBOT(const RuleCubeItemMBOT* item, int dimensionIndex,
                              ChartManager &manager)
{
  RuleCubeItemMBOT *newItem = new RuleCubeItemMBOT(item, dimensionIndex);
  std::pair<ItemSetMBOT::iterator, bool> result = m_mbotCovered.insert(newItem);
  if (!result.second) {
    delete newItem;  // already seen it
  } else {
	//Fabienne Braune : TODO : this has to be adapted for lazy scoring
    if (StaticData::Instance().GetCubePruningLazyScoring()) {
      newItem->EstimateScore();
    } else {
      	if(newItem->GetTranslationDimension().HasMoreMatchingTargetPhrase())
      	{
      		while(newItem->GetTranslationDimension().GetPosition() != newItem->GetTranslationDimension().GetPositionOfMatchingTargetPhrase())
      		{
      			newItem->IncrementTranslationDimension();
      		}
      		//std::cerr << "WE HAVE A MATCHING TARGET PHRASE AT POSITION : " << newItem->GetTranslationDimension().GetPosition() << std::endl;
      	    newItem->CreateHypothesis(m_mbotTransOpt, manager);
      	}
    }
    m_mbotQueue.push(newItem);
  }
}

}

