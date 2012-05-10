// $Id$
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

#include "ChartTranslationOption.h"

#include "AlignmentInfo.h"
#include "ChartCellCollection.h"
#include "DotChart.h"

#include <vector>

namespace Moses
{

void ChartTranslationOption::CalcEstimateOfBestScore(
    const ChartCellCollection &allChartCells)
{
  const TargetPhrase &targetPhrase = **(m_targetPhraseCollection.begin());
  m_estimateOfBestScore = targetPhrase.GetFutureScore();

  const DottedRule *rule = &m_dottedRule;

  // only deal with non-terminals
  while (!rule->IsRoot()) {
    if (rule->IsNonTerminal()) {
      // add the score of the best underlying hypothesis
      const ChartCellLabel &cellLabel = rule->GetChartCellLabel();
      const ChartHypothesisCollection *hypoColl = cellLabel.GetStack();
      CHECK(hypoColl);
      m_estimateOfBestScore += hypoColl->GetBestScore();
    }
    rule = rule->GetPrev();
  }
}

}
