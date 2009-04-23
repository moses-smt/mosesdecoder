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

#include <algorithm>
#include <iostream>

#include "TypeDef.h"

#include "FeatureFunction.h"
#include "Gibbler.h"

namespace Moses {
  class Sample;
};

namespace Josiah {

  typedef std::vector<const Moses::Factor*> TagSequence;



/**
  * Abstract base class for feature functions which use source/target pos tags.
  **/
class PosFeatureFunction : public  FeatureFunction {
  public:
    PosFeatureFunction(const std::string& name, Moses::FactorType sourceFactorType, Moses::FactorType targetFactorType) 
        : FeatureFunction(name), m_sourceFactorType(sourceFactorType), m_targetFactorType(targetFactorType) {
      //assert(targetFactorType < StaticData::Instance().GetMaxNumFactors(Output));
      //assert(sourceFactorType < StaticData::Instance().GetMaxNumFactors(Input));
    }
    //These methods must be implemented by a subclass
    /** Full score of sample*/
    virtual float computeScore(const TagSequence& sourceTags, const TagSequence& targetTags) const = 0;
    /**Change in score when updating one segment*/
    virtual float getSingleUpdateScore(
                                       const Moses::WordsRange& sourceSegment, const Moses::WordsRange& targetSegment, 
                                       const TagSequence& newTargetTags) const = 0;
    /**Change in score when flipping two segments*/
    virtual float getFlipUpdateScore(const std::pair<Moses::WordsRange,Moses::WordsRange>& sourceSegments,
                                       const std::pair<Moses::WordsRange,Moses::WordsRange>& targetSegments) const = 0;
    
    /** Tags currently in this segment*/
    void getCurrentTargetTags(const WordsRange& targetSegment, TagSequence& tags) const;
    /** All tags */
    void getCurrentTargetTags(TagSequence& tags) const;
    
    /** Initialise */
    virtual void init(const Moses::Sample& sample) {m_sample = &sample;}
    /** Compute full score of a sample from scratch **/
    virtual float computeScore();
    /** Change in score when updating one segment */
    virtual float getSingleUpdateScore(const Moses::TranslationOption* option, const Moses::WordsRange& targetSegment);
    /** Change in score when updating two segments **/
    virtual float getPairedUpdateScore(const TranslationOption* leftOption,
                                       const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase);
    
    /** Change in score when flipping */
    virtual float getFlipUpdateScore(const Moses::TranslationOption* leftTgtOption, 
                                     const Moses::TranslationOption* rightTgtOption, 
                                     const Moses::Hypothesis* leftTgtHyp, const Moses::Hypothesis* rightTgtHyp, 
                                     const Moses::WordsRange& leftTargetSegment, const Moses::WordsRange& rightTargetSegment);
    virtual ~PosFeatureFunction() {}
    
  protected:
    const TagSequence& getSourceTags() {return m_sourceTags;}
  
  private:
    TagSequence m_sourceTags;
    FactorType m_sourceFactorType;
    FactorType m_targetFactorType;
    const Moses::Sample* m_sample;
};


//FIXME: These should be configurable because they will change for different tag sets.
struct SourceVerbPredicate {
  bool operator()(const Factor* tag);
};

struct TargetVerbPredicate {
  bool operator()(const Factor* tag);
};


/**
 * Feature which counts the difference between the verb counts on each side (target-source).
**/
class VerbDifferenceFeature: public  PosFeatureFunction {
  public:
  VerbDifferenceFeature(FactorType sourceFactorType, FactorType targetFactorType) :
      PosFeatureFunction("VerbDifference", sourceFactorType, targetFactorType) {}

  
    virtual float computeScore(const TagSequence& sourceTags, const TagSequence& targetTags) const;
    virtual float getSingleUpdateScore ( 
          const Moses::WordsRange& sourceSegment, const Moses::WordsRange& targetSegment, 
          const TagSequence& newTargetTags) const;
    virtual float getFlipUpdateScore(const std::pair<Moses::WordsRange,Moses::WordsRange>& sourceSegments,
                                     const std::pair<Moses::WordsRange,Moses::WordsRange>& targetSegments) const
          {return 0;} //flipping can't change the verb difference
    
    virtual ~VerbDifferenceFeature() {}
    
  private:
    
  
};

}



