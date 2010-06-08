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

#include "Gibbler.h"
#include "GibbsOperator.h"

namespace Josiah {

FValue DistortionPenaltyFeature::computeScore() {
  //FIXME: To be useful for consistency checking, this needs to be computed
  //from scratch each time.
  const Hypothesis* sampleHypo = m_sample->GetSampleHypothesis();
  const ScoreComponentCollection& mosesScores = sampleHypo->GetScoreBreakdown();
  const ScoreProducer* distortionProducer = StaticData::Instance().GetDistortionScoreProducer();
  return mosesScores.GetScoreForProducer(distortionProducer);
}


FValue DistortionPenaltyFeature::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap) 
{
  FValue distortion;
  CheckValidReordering(leftOption->GetSourceWordsRange(), rightOption->GetSourceWordsRange(), 
                       leftGap.leftHypo, leftGap.rightHypo, 
                       rightGap.leftHypo, rightGap.rightHypo, distortion);
  return distortion;
  
}

}
