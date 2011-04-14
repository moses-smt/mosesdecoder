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
#include "TargetPhrase.h"
#include "AlignmentInfo.h"
#include "CoveredChartSpan.h"
#include "ChartCellCollection.h"

using namespace std;

namespace Moses
{

void ChartTranslationOption::CreateNonTermIndex()
{
  m_coveredChartSpanListTargetOrder.resize(m_targetPhrase.GetSize(), NOT_FOUND);
  const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();

  size_t nonTermInd = 0;
  size_t prevSourcePos = 0;
  AlignmentInfo::const_iterator iter;
  for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter) {
    // alignment pairs must be ordered by source index
    size_t sourcePos = iter->first;
    if (nonTermInd > 0) {
      assert(sourcePos > prevSourcePos);
    }
    prevSourcePos = sourcePos;

    size_t targetPos = iter->second;
    m_coveredChartSpanListTargetOrder[targetPos] = nonTermInd;
    nonTermInd++;
  }
}

void ChartTranslationOption::CalcEstimateOfBestScore(
  const CoveredChartSpan *coveredChartSpan,
  const ChartCellCollection &allChartCells)
{
  // recurse through the linked list of source side non-terminals and terminals
  const CoveredChartSpan *prevCoveredChartSpan =
    coveredChartSpan->GetPrevCoveredChartSpan();
  if (prevCoveredChartSpan)
  {
    CalcEstimateOfBestScore(prevCoveredChartSpan, allChartCells);
  }

  // only deal with non-terminals
  if (coveredChartSpan->IsNonTerminal())
  {
    // get the essential information about the non-terminal
    const WordsRange &childRange = coveredChartSpan->GetWordsRange();
    const ChartCell &childCell = allChartCells.Get(childRange);
    const Word &nonTerm = coveredChartSpan->GetSourceWord();

    // there have to be hypotheses with the desired non-terminal
    // (otherwise the rule would not be considered)
    assert(!childCell.GetSortedHypotheses(nonTerm).empty());

    // create a list of hypotheses that match the non-terminal
    const vector<const ChartHypothesis *> &stack =
      childCell.GetSortedHypotheses(nonTerm);
    const ChartHypothesis *hypo = stack[0];
    m_estimateOfBestScore += hypo->GetTotalScore();
  }
}

std::ostream& operator<<(std::ostream &out, const ChartTranslationOption &rule)
{
  out << rule.m_lastCoveredChartSpan << ": " << rule.m_targetPhrase.GetTargetLHS() << "->" << rule.m_targetPhrase;
  return out;
}

} // namespace

