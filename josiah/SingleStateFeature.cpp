/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011 University of Edinburgh

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

#include "SingleStateFeature.h"

#include "Gibbler.h"

using namespace Moses;
using namespace std;

namespace Josiah {

FeatureFunctionHandle SingleStateFeature::getFunction
    (const Sample& sample) const {
  return FeatureFunctionHandle(
    new SingleStateFeatureFunction(sample,m_mosesFeature)); 
}

void SingleStateFeatureFunction::assignScore(FVector& scores) {
  //Use the score cached by updateTarget()
  scores += m_accumulator.GetScoresVector();
}


void SingleStateFeatureFunction::updateTarget() {
  //Update the prevStates map, and the cached scores
    
  m_prevStates.clear();
  m_accumulator.ZeroAll();
  const Moses::Hypothesis* currHypo = getSample().GetTargetTail();
  StateHandle prevState(
    m_mosesFeature->EmptyHypothesisState(currHypo->GetInput()));
  while ((currHypo = (currHypo->GetNextHypo()))) {
    StateHandle currState(m_mosesFeature->Evaluate(
      *currHypo, prevState.get(), &m_accumulator));
    for (size_t i = 0; i < currHypo->GetCurrTargetWordsRange().
        GetNumWordsCovered(); ++i) {
      m_prevStates.push_back(prevState);
    }
    prevState = currState;
  }
}


/** Score due to one segment */
void SingleStateFeatureFunction::doSingleUpdate(
    const TranslationOption* option,
    const TargetGap& gap, FVector& scores) {
  
  ScoreComponentCollection accumulator;
  //the previous state of the (new) hypo
  StateHandle prevState = m_prevStates[gap.segment.GetStartPos()];
  //Evaluate the score of inserting this hypo, and get the prev state
  //for the next hypo.
  prevState.reset(m_mosesFeature->Evaluate(
    *option,prevState.get(),&accumulator));
  //if there's a hypo on the right, then evaluate it
  if (gap.rightHypo) {
    prevState.reset(m_mosesFeature->Evaluate(
      gap.rightHypo->GetTranslationOption(),prevState.get(),&accumulator));
  }
  scores += accumulator.GetScoresVector();
}

/** Score due to two segments.
   The left and right refer to the target positions.**/
void SingleStateFeatureFunction::doContiguousPairedUpdate(
  const TranslationOption* leftOption,
  const TranslationOption* rightOption, 
  const TargetGap& gap, FVector& scores) {
  ScoreComponentCollection accumulator;
  //The previous state of the (new) current hypo.
  StateHandle prevState(m_prevStates[gap.segment.GetStartPos()]);
  //Evaluate the hypos in the gap
  prevState.reset(m_mosesFeature->Evaluate(
    *leftOption,prevState.get(),&accumulator));
  prevState.reset(m_mosesFeature->Evaluate(
    *rightOption,prevState.get(),&accumulator));
  //if there's a hypo on the right, evaluate it
  if (gap.rightHypo) {
    prevState.reset(m_mosesFeature->Evaluate(
      gap.rightHypo->GetTranslationOption(),prevState.get(),&accumulator));
  }
  scores += accumulator.GetScoresVector();
  
}


void SingleStateFeatureFunction::doDiscontiguousPairedUpdate(
  const TranslationOption* leftOption,
  const TranslationOption* rightOption, 
  const TargetGap& leftGap,
  const TargetGap& rightGap,
  FVector& scores) {
  doSingleUpdate(leftOption,leftGap,scores);
  doSingleUpdate(rightOption,rightGap,scores);
}

/** Score due to flip.
  Again, left and right refer to order on the <emph>target</emph> side. */
void SingleStateFeatureFunction::doFlipUpdate(
  const TranslationOption* leftOption,
  const TranslationOption* rightOption, 
  const TargetGap& leftGap,
  const TargetGap& rightGap,
  FVector& scores) {
  if (leftGap.segment.GetEndPos() + 1 == rightGap.segment.GetStartPos()) {
    TargetGap gap(leftGap.leftHypo,rightGap.rightHypo,
      WordsRange(leftGap.segment.GetStartPos(),rightGap.segment.GetEndPos()));
    doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
  } else {
    doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
  }

}

}
