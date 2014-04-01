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

  m_mbotEstimateOfBestScore = mbotTp->GetFutureScore();

  const DottedRuleMBOT *rule = &m_mbotDottedRule;

  // only deal with non-terminals
  while (!rule->IsRootMBOT()) {
    if (rule->IsNonTerminalMBOT()) {
      // add the score of the best underlying hypothesis
      const ChartCellLabelMBOT &cellLabel = rule->GetChartCellLabelMBOT();
      const ChartHypothesisCollectionMBOT *hypoColl = cellLabel.GetStackMBOT();
      CHECK(hypoColl);
      m_mbotEstimateOfBestScore += hypoColl->GetBestScore();
    }
    rule = rule->GetPrevMBOT();
  }
}

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
    out << coll.GetEstimateOfBestScoreMBOT() << std::endl;
    return out;
}
}//namespace
