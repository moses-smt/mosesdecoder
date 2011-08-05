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
#include <sstream>
#include <string>

#include "LexicalReorderingFeature.h"
#include "ScoreComponentCollection.h"

using namespace Moses;
using namespace std;

namespace Josiah {

LexicalReorderingFeature::LexicalReorderingFeature
  (Moses::LexicalReordering* lexReorder,size_t index) :
    m_mosesLexReorder(lexReorder),
    m_index(index) {
  size_t featureCount = m_mosesLexReorder->GetNumScoreComponents();
  const string& root = "LexicalReordering";
  for (size_t i = 1; i <= featureCount; ++i) {
    ostringstream namestream;
    if (index > 0) {
      namestream << index << "-";
    }
    namestream << i;
    m_featureNames.push_back(FName(root,namestream.str()));
  }
  
}

FeatureFunctionHandle LexicalReorderingFeature::getFunction(const Sample& sample) const {
  return FeatureFunctionHandle(
    new LexicalReorderingFeatureFunction(sample,m_featureNames,m_mosesLexReorder));
}

LexicalReorderingFeatureFunction::LexicalReorderingFeatureFunction
  (const Sample& sample, std::vector<FName> featureNames,
    LexicalReordering* lexReorder):
    FeatureFunction(sample),
    m_featureNames(featureNames),
    m_mosesLexReorder(lexReorder) {
}

/** Assign the total score of this feature on the current hypo */
void LexicalReorderingFeatureFunction::assignScore(FVector& scores) {
  
  /*
  const Hypothesis * currHypo = getSample().GetTargetTail();
  const FFState* state = m_mosesLexReorder->EmptyHypothesisState(currHypo->GetInput());
  ScoreComponentCollection accumulator;
  cerr << *currHypo << endl;
  while ((currHypo = (currHypo->GetNextHypo()))) {
    state = m_mosesLexReorder->Evaluate(*currHypo,state,&accumulator);
    cerr << "AS: " << accumulator << endl;
  }*/
  vector<float> mosesScores = m_accumulator.GetScoresForProducer(m_mosesLexReorder);
  for (size_t i = 0; i < m_featureNames.size(); ++i) {
    scores[m_featureNames[i]] = mosesScores[i];
  }

}

/** Update the  previous state map.*/
void LexicalReorderingFeatureFunction::updateTarget() {
  m_prevStates.clear();
  m_accumulator.ZeroAll();
  const Hypothesis * currHypo = getSample().GetTargetTail();
  LRStateHandle prevState(dynamic_cast<const LexicalReorderingState*>(m_mosesLexReorder->EmptyHypothesisState(currHypo->GetInput())));
  while ((currHypo = (currHypo->GetNextHypo()))) {
    LRStateHandle currState(dynamic_cast<const LexicalReorderingState*>(m_mosesLexReorder->Evaluate(currHypo->GetTranslationOption(),prevState.get(),&m_accumulator)));
    for (size_t i = 0; i < currHypo->GetCurrTargetWordsRange().GetNumWordsCovered(); ++i) {
      m_prevStates.push_back(prevState);
    }
    prevState = currState;
  }
  
}

void LexicalReorderingFeatureFunction::addScore
  (vector<float>& accumulator, FVector& scores) {
  for (size_t i = 0; i < accumulator.size(); ++i) {
    scores[m_featureNames[i]] += accumulator[i];
    accumulator[i] = 0;
  }
}
    
/** Score due to one segment */
void LexicalReorderingFeatureFunction::doSingleUpdate
  (const TranslationOption* option, const TargetGap& gap, FVector& scores) {
  vector<float> accumulator(m_mosesLexReorder->GetNumScoreComponents(),0);
  //The previous state of the (new) current hypo.
  LRStateHandle prevState = m_prevStates[gap.segment.GetStartPos()];
  //Evaluate the score of inserting this hypo, and get the prev state 
  //for the next hypo.
  prevState.reset(prevState->Expand(*option,accumulator));
  addScore(accumulator,scores);
  //if there's a hypo on the right, then evaluate it.
  if (gap.rightHypo) {
    prevState.reset(prevState->Expand(gap.rightHypo->GetTranslationOption(),accumulator));
    addScore(accumulator,scores);
  }
  
}

/* Score due to two segments. The left and right refer to the target positions.**/
void LexicalReorderingFeatureFunction::doContiguousPairedUpdate
  (const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap, FVector& scores) {
  vector<float> accumulator(m_mosesLexReorder->GetNumScoreComponents(),0);
  //The previous state of the (new) current hypo.
  LRStateHandle prevState(m_prevStates[gap.segment.GetStartPos()]);
  //Evaluate the hypos in the gap
  prevState.reset(prevState->Expand(*leftOption,accumulator));
  addScore(accumulator,scores);
  prevState.reset(prevState->Expand(*rightOption,accumulator));
  addScore(accumulator,scores);
  //if there's a hypo on the right, then evaluate it.
  if (gap.rightHypo) {
    prevState.reset(prevState->Expand(gap.rightHypo->GetTranslationOption(),accumulator));
    addScore(accumulator,scores);
  }

}

void LexicalReorderingFeatureFunction::doDiscontiguousPairedUpdate
(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
  doSingleUpdate(leftOption,leftGap, scores);
  doSingleUpdate(rightOption,rightGap, scores);
}
    
/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
void LexicalReorderingFeatureFunction::doFlipUpdate(
  const TranslationOption* leftOption,
  const TranslationOption* rightOption, 
  const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) {
  if (leftGap.segment.GetEndPos() + 1 == rightGap.segment.GetStartPos()) {
    TargetGap gap(leftGap.leftHypo,rightGap.rightHypo,
      WordsRange(leftGap.segment.GetStartPos(),rightGap.segment.GetEndPos()));
    doContiguousPairedUpdate(leftOption,rightOption,gap,scores);
  } else {
    doDiscontiguousPairedUpdate(leftOption,rightOption,leftGap,rightGap,scores);
  }
}
    

}

