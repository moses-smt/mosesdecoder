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
#include "RuleCubeItemMBOT.h"
#include "ChartHypothesisMBOT.h"
#include "ChartHypothesisCollection.h"
#include "RuleCubeQueue.h"
#include "WordsRange.h"
#include "Util.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

RuleCubeItemMBOT::RuleCubeItemMBOT( const ChartTranslationOptions &transOpt,
                                    const ChartCellCollection &allChartCells)
                                    : RuleCubeItem(transOpt, allChartCells)
                                    , m_mbotTranslationDimension(0,transOpt.GetTargetPhraseCollection().GetCollection())
                                    , m_mbotHypothesis(0)
{
  CreateHypothesisDimensionsMBOT(transOpt.GetStackVec());
}

//new : create the RuleCubeMBOT from an existing one, differing only in one dimension
RuleCubeItemMBOT::RuleCubeItemMBOT(const RuleCubeItemMBOT &copy, int hypoDimensionIncr)
  : RuleCubeItem(copy,hypoDimensionIncr)
  , m_mbotTranslationDimension(copy.m_mbotTranslationDimension)
  , m_hypothesisDimensions(copy.m_hypothesisDimensions)
  , m_mbotHypothesis(0)
{
    //std::cout << "Create Rule Cube from existing Dimension"<< std::endl;

  if (hypoDimensionIncr == -1) {
    m_mbotTranslationDimension.IncrementPos();
  } else {
    HypothesisDimension &dimension = m_hypothesisDimensions[hypoDimensionIncr];
    dimension.IncrementPos();
  }
}

RuleCubeItemMBOT::~RuleCubeItemMBOT()
{
  delete m_mbotHypothesis;
}

void RuleCubeItemMBOT::EstimateScoreMBOT()
{
  m_mbotScore = m_mbotTranslationDimension.GetTargetPhrase()->GetFutureScore();
  std::vector<HypothesisDimension>::const_iterator p;
  for (p = m_hypothesisDimensions.begin();
       p != m_hypothesisDimensions.end(); ++p) {
    m_mbotScore += p->GetHypothesis()->GetTotalScore();
  }
}

ChartHypothesisMBOT *RuleCubeItemMBOT::ReleaseHypothesisMBOT()
{
  ChartHypothesisMBOT *hypo = m_mbotHypothesis;
  m_mbotHypothesis = 0;
  return hypo;
}

void RuleCubeItemMBOT::CreateHypothesis(const ChartTranslationOptions &transOpt,
                                    ChartManager &manager)
{

  m_mbotHypothesis = new ChartHypothesisMBOT(transOpt, *this, manager);
  m_mbotHypothesis->CalcScoreMBOT();
  m_mbotScore = m_mbotHypothesis->GetTotalScore();
  //Fabienne Braune : usefull for using debugging GUI
  /*std::cerr << "-----------------------" << std::endl;
  std::cerr << "RCI : CREATING HYPO : " << std::endl;
  std::cerr << "-----------------------" << std::endl;
  std::cerr << (*m_mbotHypothesis) << std::endl;*/
  //std::cout << m_mbotHypothesis->GetTranslationOptionMBOT().GetDottedRuleMBOT() << std::endl;
 //std::cerr << m_mbotHypothesis->GetCurrTargetPhraseMBOT() << std::endl;
}

// for each non-terminal, create a ordered list of matching hypothesis from the chart
void RuleCubeItemMBOT::CreateHypothesisDimensionsMBOT(const StackVec &stackVec)
{
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end();
       ++p) {
    const HypoList *stack = (*p)->GetStack().cube;
    assert(stack);
    assert(!stack->empty());
    HypothesisDimension dimension(0, *stack);
    m_hypothesisDimensions.push_back(dimension);
  }
}

//Fabienne Braune : TODO : remove if CreateHypothesesDimensions works
/*void RuleCubeItemMBOT::CreateHypothesisDimensionsMBOT(
  const DottedRuleMBOT &dottedRule,
  const ChartCellCollection &allChartCells)
{
  //std::cout << "RCIMBOT: CREATING HYPOTHESIS DIMENSION FOR RULE : " << dottedRule << std::endl;
  CHECK(!dottedRule.IsRootMBOT());

  //std::cout << "DOTTED RULE FOR DIMENSION " << dottedRule<< std::endl;
  const DottedRuleMBOT *prev = dottedRule.GetPrevMBOT();
  //std::cout << "PREVIOUS DOTTED RULE FOR DIMENSION " << (*prev) << std::endl;

  if (!prev->IsRootMBOT()) {
     //std::cout << "RCIMBOT: Previous is not root" << std::endl;
    CreateHypothesisDimensionsMBOT(*prev, allChartCells);
  }
  //std::cout << "RCIMBOT: Previous is root" << std::endl;

  // only deal with non-terminals
  if (dottedRule.IsNonTerminalMBOT()) {
    //std::cout << "RCIMBOT: Word is non terminal" << std::endl;
    // get a sorted list of the underlying hypotheses
    const ChartCellLabelMBOT &cellLabel = dottedRule.GetChartCellLabelMBOT();
    //std::cout << "FOR CELL LABEL : " << cellLabel << std::endl;

    //std::cout << "Check that Stack is CORRECT" << std::endl;
    const ChartHypothesisCollectionMBOT *hypoColl = cellLabel.GetStackMBOT();
    //std::cout << "HYPO COLL : " << (*hypoColl) << std::endl;

    //new : inserted for testing : Requires to make HCType public !
    //ChartHypothesisCollectionMBOT::HCTypeMBOT::const_iterator iter;
    //for(iter = hypoColl->begin(); iter != hypoColl->end(); iter++)
    //{
    //    const ChartHypothesisMBOT &testHypo = **iter;
    //    std::cout << "HYPO : " << testHypo << std::endl;
   //}

    CHECK(hypoColl);
    //std::cout << "RCIMBOT: Getting sorted hyotheses" << std::endl;
    const HypoListMBOT &hypoList = hypoColl->GetSortedHypothesesMBOT();

    // there have to be hypothesis with the desired non-terminal
    // (otherwise the rule would not be considered)
    CHECK(!hypoList.empty());

    // create a list of hypotheses that match the non-terminal
    HypothesisDimensionMBOT dimension(0, hypoList);
    //std::cout << "RCI : Created hypo dimension : " << (*dimension.GetHypothesis()) << std::endl;
    m_mbotHypothesisDimensions.push_back(dimension);
  }
 //std::cout << "HYPOTHESIS DIMENSION CREATED END" << std::endl;
}*/

bool RuleCubeItemMBOT::operator<(const RuleCubeItemMBOT &compare) const
{
  //std::cout << "RCIM : Comparing translation dimensions : " << std::endl;
  if (m_mbotTranslationDimension == compare.m_mbotTranslationDimension) {
    return m_hypothesisDimensions < compare.m_hypothesisDimensions;
  }
  return m_mbotTranslationDimension < compare.m_mbotTranslationDimension;
}

}
