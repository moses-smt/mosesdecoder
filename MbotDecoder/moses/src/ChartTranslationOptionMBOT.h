// $Id: ChartTranslationOptionMBOT.h,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#pragma once

#include "TargetPhraseMBOT.h"
#include "TargetPhraseCollection.h"
#include "WordsRange.h"

#include "DotChart.h"
#include "ChartCellCollection.h"

#include "DotChartMBOT.h"

#include "util/check.hh"
#include <vector>

#include <boost/shared_ptr.hpp>

namespace Moses
{

typedef boost::shared_ptr<TargetPhraseMBOT> targetPhrasePtr;

// Similar to a DottedRule, but contains a direct reference to a list
// of translations and provdes an estimate of the best score.
class ChartTranslationOptionMBOT : public ChartTranslationOption
{

 friend std::ostream& operator<<(std::ostream&, const ChartTranslationOptionMBOT&);

 public:
 ChartTranslationOptionMBOT(
                         const TargetPhraseCollection &targetPhraseColl,
                         const DottedRuleMBOT &dottedRule,
                         const WordsRange &wordsRange,
                         const ChartCellCollection &allChartCells)
                         : ChartTranslationOption(targetPhraseColl,dottedRule,wordsRange,allChartCells)
                         , m_mbotDottedRule(dottedRule)

{
    //std::cout << "new ChartTranslationOptionMBOT() " << std::endl;
    //std::cout << "Adress of Target Phrase Collection " << &targetPhraseColl << std::endl;
	 CalcEstimateOfBestScoreMBOT(allChartCells);

    /*const TargetPhraseCollection tpc = GetTargetPhraseCollection();
    std::cout << "CTOMBOT AFTER ESTIMATE : TARGET NOT EMPTY : " << std::endl;
        TargetPhrase * tp = *(tpc.begin());
        //cast to target phrase MBOT
        TargetPhraseMBOT * targetPhrase = static_cast<TargetPhraseMBOT*>(tp);
        std::cout << "CTOMBOT AFTER ESTIMATE : Adress : " << &targetPhrase << *targetPhrase << std::endl;*/
  }

  //! forbid access to non-mbot dotted rule
  const DottedRule &GetDottedRule() const
  {
     std::cout << "Get non mbot dotted rule NOT IMPLEMENTED in ChartTranslationOptionMBOT" << std::endl;
  }

  // return an estimate of the best score possible with this translation option.
  // the estimate is the sum of the top target phrase's estimated score plus the
  // scores of the best child hypotheses.
 inline float GetEstimateOfBestScore() const {
    std::cout << "Get estimate of best score NOT IMPLEMENTED in ChartTranslationOptionMBOT" << std::endl;
}

 inline float GetEstimateOfBestScoreMBOT() const {
     return m_mbotEstimateOfBestScore;
}

  const DottedRuleMBOT &GetDottedRuleMBOT() const { return m_mbotDottedRule; }

  void ReEstimateBestScoreMatchLabels(const ChartCellCollection &);

  protected:
  // not implemented
  ChartTranslationOptionMBOT &operator=(const ChartTranslationOptionMBOT &);
  void CalcEstimateOfBestScoreMBOT(const ChartCellCollection &);
  const DottedRuleMBOT &m_mbotDottedRule;
  float m_mbotEstimateOfBestScore;
};

}
