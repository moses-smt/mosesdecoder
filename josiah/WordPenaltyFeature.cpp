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


#include "WordPenaltyFeature.h"

#include "DummyScoreProducers.h"
#include "Gibbler.h"
#include "StaticData.h"

namespace Josiah {

WordPenaltyFeature::WordPenaltyFeature() :
  SingleValuedFeatureFunction(StaticData::Instance().GetWordPenaltyProducer()->GetScoreProducerDescription()) {}
  
FValue WordPenaltyFeature::computeScore() {
  return -m_sample->GetTargetWords().size();
}

/** Score due to one segment */
FValue WordPenaltyFeature::getSingleUpdateScore(const TranslationOption* option, const TargetGap& gap) {
  return -(option->GetTargetPhrase().GetSize());
}

/** Score due to two segments. The left and right refer to the target positions.**/
FValue WordPenaltyFeature::getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& gap) {
  return -(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
  
}

FValue WordPenaltyFeature::getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
    const TargetGap& leftGap, const TargetGap& rightGap) {
      return -(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
}


/** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
FValue WordPenaltyFeature::getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap) {
   return -(leftOption->GetTargetPhrase().GetSize() + rightOption->GetTargetPhrase().GetSize());
}
}//namespace