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

#include "ChartTranslationOptions.h"

#include "ChartHypothesis.h"

#include "ChartCellLabel.h"

namespace Moses
{

float ChartTranslationOptions::CalcEstimateOfBestScore(
    const TargetPhraseCollection &tpc,
    const StackVec &stackVec)
{
  const TargetPhrase &targetPhrase = **(tpc.begin());
  float estimateOfBestScore = targetPhrase.GetFutureScore();
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end();
       ++p) {
    const HypoList *stack = (*p)->GetStack().cube;
    assert(stack);
    assert(!stack->empty());
    const ChartHypothesis &bestHypo = **(stack->begin());
    estimateOfBestScore += bestHypo.GetTotalScore();
  }
  return estimateOfBestScore;
}

}
