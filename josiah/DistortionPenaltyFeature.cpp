/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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


#include "DistortionPenaltyFeature.h"
#include "DummyScoreProducers.h"

#include "Derivation.h"
#include "Gibbler.h"
#include "GibbsOperator.h"


namespace Josiah {

FeatureFunctionHandle DistortionPenaltyFeature::getFunction( const Sample& sample ) const {
  return FeatureFunctionHandle(new DistortionPenaltyFeatureFunction(sample));
}

FValue DistortionPenaltyFeatureFunction::computeScore() {
  FValue distortion = 0;
  //cerr << Derivation(*m_sample) << endl;
  const Hypothesis* currHypo = getSample().GetTargetTail(); //target tail
  
  //step through in target order
  int lastSrcEnd = -1;
  while ((currHypo = (currHypo->GetNextHypo()))) {
    int srcStart = currHypo->GetCurrSourceWordsRange().GetStartPos();
    distortion -= abs(srcStart - (lastSrcEnd+1));
    lastSrcEnd = currHypo->GetCurrSourceWordsRange().GetEndPos();
  }
  //cerr << "distortion " << distortion << endl;
  return distortion;
}


FValue DistortionPenaltyFeatureFunction::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap) 
{
  FValue distortion;
  const Hypothesis* leftTgtNextHypo = leftGap.rightHypo;
  const Hypothesis* rightTgtPrevHypo = rightGap.leftHypo;
  //if the segments are contiguous and we're swapping, then these hypos have to be swapped so 
  //that they're in the order they'd appear in in the proposed target
  if (leftGap.segment.GetEndPos()+1 == rightGap.segment.GetStartPos()) {
    if (leftTgtNextHypo->GetCurrSourceWordsRange() != rightOption->GetSourceWordsRange()) {
      const Hypothesis* tmp = leftTgtNextHypo;
      leftTgtNextHypo = rightTgtPrevHypo;
      rightTgtPrevHypo = tmp;
    }
  }
  CheckValidReordering(leftOption->GetSourceWordsRange(), rightOption->GetSourceWordsRange(), 
                       leftGap.leftHypo, leftTgtNextHypo, 
                       rightTgtPrevHypo, rightGap.rightHypo, distortion);
  //cerr << leftOption->GetSourceWordsRange() << " " << rightOption->GetSourceWordsRange() << " " << distortion << endl;
  //cerr << "lg.rh" << leftTgtNextHypo->GetCurrSourceWordsRange() << " rg.lh" << rightTgtPrevHypo->GetCurrSourceWordsRange() << endl;
  return distortion;
  
}

}
