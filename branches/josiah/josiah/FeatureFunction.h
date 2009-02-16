/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#pragma once

#include "Gibbler.h"
#include "Hypothesis.h"
#include "TranslationOptionCollection.h"

using namespace Moses;

namespace Moses {
  class Sample;
}

namespace Josiah {
  
  

/**
  * Base class for Gibbler feature functions.
 **/
class FeatureFunction {
  public:
    FeatureFunction(Sample& sample);
    /** Score for the sample */
    virtual float getScore() = 0;
    /** Change in score when updating one segment */
    virtual float getSingleUpdateScore(const TranslationOption* option, const WordsRange& targetSegment) = 0;
    /** Change in score when updating two segments **/
    virtual float getPairedUpdateScore(const TranslationOption* leftOption,
                               const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) = 0;
    /** Change in score when flipping */
    virtual float getFlipUpdateScore(const TranslationOption* leftTgtOption, const TranslationOption* rightTgtOption, 
                             const Hypothesis* leftTgtHyp, const Hypothesis* rightTgtHyp, 
                             const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) = 0;
    virtual ~FeatureFunction();
    
  protected:
    Sample& getSample() {return m_sample;}
    
    
  private:
    Sample& m_sample;
    
    
};

} //namespace
