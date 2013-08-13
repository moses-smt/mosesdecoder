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
#include "ChartTranslationOption.h"

namespace Moses
{

ChartTranslationOptions::ChartTranslationOptions(const TargetPhraseCollection &targetPhraseColl,
    const StackVec &stackVec,
    const WordsRange &wordsRange,
    float score)
  : m_stackVec(stackVec)
  , m_wordsRange(&wordsRange)
  , m_estimateOfBestScore(score)
{
  TargetPhraseCollection::const_iterator iter;
  for (iter = targetPhraseColl.begin(); iter != targetPhraseColl.end(); ++iter) {
    const TargetPhrase *origTP = *iter;

    boost::shared_ptr<ChartTranslationOption> ptr(new ChartTranslationOption(*origTP));
    m_collection.push_back(ptr);
  }
}

ChartTranslationOptions::~ChartTranslationOptions()
{

}

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

void ChartTranslationOptions::Evaluate(const InputPath &inputPath, const InputType &input)
{
  CollType::iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter) {
	  ChartTranslationOption &transOpt = **iter;
	  transOpt.Evaluate(inputPath, input);
  }

}

}
