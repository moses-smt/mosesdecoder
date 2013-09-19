// $Id: ChartTranslationOptionMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
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

#include "ChartTranslationOptionMBOT.h"
#include "AlignmentInfo.h"
#include "ChartCellCollection.h"
#include "DotChart.h"
#include "Word.h"

#include <vector>

namespace Moses
{

void ChartTranslationOptionMBOT::CalcEstimateOfBestScoreMBOT(
    const ChartCellCollection &allChartCells)
{
        TargetPhrase * tp = *(GetTargetPhraseCollection().begin());
        //cast to target phrase MBOT
        TargetPhraseMBOT * mbotTp = static_cast<TargetPhraseMBOT*>(tp);
        //std::cout << "TARGET PHRASE : " << std::endl;
        //std::cout << (*mbotTp) << std::endl;
        //std::cout << "CTOMBOT : Adress : " << &targetPhrase << *targetPhrase << std::endl;

  m_mbotEstimateOfBestScore = mbotTp->GetFutureScore();
  //std::cout << "FIRST ESTIMATE SCORE FOR CHART TRANSLATION OPTION " << m_mbotEstimateOfBestScore << std::endl;

  const DottedRuleMBOT *rule = &m_mbotDottedRule;
  //std::cout << "ASSOCIATED RULE : " << (*rule) << std::endl;

  // only deal with non-terminals
  while (!rule->IsRootMBOT()) {
    if (rule->IsNonTerminalMBOT()) {
      // add the score of the best underlying hypothesis
      const ChartCellLabelMBOT &cellLabel = rule->GetChartCellLabelMBOT();
      //std::cout << "CHART CELL LABEL" << std::endl;
      //std::cout << cellLabel.GetCoverageMBOT().front() << std::endl;
      //std::cout << cellLabel.GetLabelMBOT().front() << std::endl;
      const ChartHypothesisCollectionMBOT *hypoColl = cellLabel.GetStackMBOT();
      //std::cout << "ASSOCIATED HYPO COLL" << (*hypoColl) << std::endl;
      CHECK(hypoColl);
      //std::cout << "SCORE OBTAINED FOR HYPO COLL" << hypoColl->GetBestScore() << std::endl;
      m_mbotEstimateOfBestScore += hypoColl->GetBestScore();
      //std::cout << "ESTIMATED SCORE FOR CHART TRANSLATION OPTION " << m_mbotEstimateOfBestScore << std::endl;
    }
    rule = rule->GetPrevMBOT();
  }
}

/*void ChartTranslationOptionMBOT::ReEstimateBestScoreMatchLabels(const ChartCellCollection &)
{
	 //check if we have something that matches a source label
	m_mbotEstimateOfBestScore = 0;

	bool foundMatch = false;
	int index = 0;

	while(!foundMatch && index < GetTargetPhraseCollection().GetSize())
	{
			TargetPhrase * tp = *(GetTargetPhraseCollection().begin() + index);
	        //cast to target phrase MBOT
	        TargetPhraseMBOT * mbotTp = static_cast<TargetPhraseMBOT*>(tp);
	        //std::cerr << "CHECKING PHRASE : " << mbotTp << std::endl;
	        if(mbotTp->isMatchesSource())
	        //std::cout << "TARGET PHRASE : " << std::endl;
	        //std::cout << (*mbotTp) << std::endl;
	        //std::cout << "CTOMBOT : Adress : " << &targetPhrase << *targetPhrase << std::endl;

	        {
	        	m_mbotEstimateOfBestScore = mbotTp->GetFutureScore();
	        	foundMatch = true;
	        }
	        index++;
	}

	  const DottedRuleMBOT *rule = &m_mbotDottedRule;
	  //std::cout << "ASSOCIATED RULE : " << (*rule) << std::endl;

	  // only deal with non-terminals
	  while (!rule->IsRootMBOT()) {
	    if (rule->IsNonTerminalMBOT()) {
	      // add the score of the best underlying hypothesis
	      const ChartCellLabelMBOT &cellLabel = rule->GetChartCellLabelMBOT();
	      //std::cout << "CHART CELL LABEL" << std::endl;
	      //std::cout << cellLabel.GetCoverageMBOT().front() << std::endl;
	      //std::cout << cellLabel.GetLabelMBOT().front() << std::endl;
	      const ChartHypothesisCollectionMBOT *hypoColl = cellLabel.GetStackMBOT();
	      //std::cout << "ASSOCIATED HYPO COLL" << (*hypoColl) << std::endl;
	      CHECK(hypoColl);
	      //std::cout << "SCORE OBTAINED FOR HYPO COLL" << hypoColl->GetBestScore() << std::endl;
	      m_mbotEstimateOfBestScore += hypoColl->GetBestScore();
	      //std::cout << "ESTIMATED SCORE FOR CHART TRANSLATION OPTION " << m_mbotEstimateOfBestScore << std::endl;
	    }
	    rule = rule->GetPrevMBOT();
	  }
}*/

std::ostream& operator<<(std::ostream &out, const ChartTranslationOptionMBOT &coll)
{
    out << coll.GetDottedRuleMBOT() << std::endl;

    TargetPhraseCollection::const_iterator itr_target;
    for(itr_target = coll.GetTargetPhraseCollection().begin(); itr_target != coll.GetTargetPhraseCollection().end(); itr_target++)
    {
        TargetPhrase * tp = *itr_target;
        //cast to target phrase MBOT
        TargetPhraseMBOT * mbotTp = static_cast<TargetPhraseMBOT*>(tp);
        out << (*mbotTp) << std::endl;
    }
    //out << "ESTIMATE OF BEST SCORE" << std::endl;*/
    out << coll.GetEstimateOfBestScoreMBOT() << std::endl;
    return out;
}
}//namespace
