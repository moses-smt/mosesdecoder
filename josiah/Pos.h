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


namespace Josiah {

  typedef std::vector<const Moses::Factor*> TagSequence;

  class Sample;

/**
  * Abstract base class for feature functions which use source/target pos tags.
  **/
class PosFeatureFunction : public  SingleValuedFeatureFunction {
  public:
    PosFeatureFunction(const Sample& sample, 
                        const std::string& name, Moses::FactorType sourceFactorType, Moses::FactorType targetFactorType) 
        : SingleValuedFeatureFunction(sample,name), m_sourceFactorType(sourceFactorType), m_targetFactorType(targetFactorType) {
      //assert(targetFactorType < StaticData::Instance().GetMaxNumFactors(Output));
      //assert(sourceFactorType < StaticData::Instance().GetMaxNumFactors(Input));
    }
    //These methods must be implemented by a subclass
    /** Full score of sample*/
    virtual float computeScore(const TagSequence& sourceTags, const TagSequence& targetTags) const = 0;
    /**Change in score when updating one segment*/
    virtual float getSingleUpdateScore(const Moses::WordsRange& sourceSegment, const Moses::WordsRange& targetSegment, 
                                       const TagSequence& newTargetTags) const = 0;
    /**Change in score when flipping two segments. Note that both pairs are in target order */
    virtual float getFlipUpdateScore(const std::pair<Moses::WordsRange,Moses::WordsRange>& sourceSegments,
                                       const std::pair<Moses::WordsRange,Moses::WordsRange>& targetSegments) const = 0;
    

    /** All tags */
    void getCurrentTargetTags(TagSequence& tags) const;
    
    /** Compute full score of a sample from scratch **/
    virtual float computeScore();
    /** Change in score when updating one segment */
    virtual float getSingleUpdateScore(const Moses::TranslationOption* option, const TargetGap& gap);
    /** Change in score when updating two segments **/
    virtual float getContiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& gap);
    virtual float getDiscontiguousPairedUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
        const TargetGap& leftGap, const TargetGap& rightGap);
    
    /** Change in score when flipping */
    virtual float getFlipUpdateScore(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                     const TargetGap& leftGap, const TargetGap& rightGap);
    virtual ~PosFeatureFunction() {}
    
  protected:
    const TagSequence& getSourceTags() {return m_sourceTags;}
  
  private:
    TagSequence m_sourceTags;
    FactorType m_sourceFactorType;
    FactorType m_targetFactorType;
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
class VerbDifferenceFeature : public Feature {
  public:
    VerbDifferenceFeature(FactorType sourceFactorType, FactorType targetFactorType);
    virtual FeatureFunctionHandle getFunction( const Sample& sample ) const;
    
  private:
    FactorType m_sourceFactorType;
    FactorType m_targetFactorType;
};
    
class VerbDifferenceFeatureFunction: public  PosFeatureFunction {
  public:
  VerbDifferenceFeatureFunction(const Sample& sample, FactorType sourceFactorType, FactorType targetFactorType) :
      PosFeatureFunction(sample, "VerbDifference", sourceFactorType, targetFactorType) {}

  
    virtual float computeScore(const TagSequence& sourceTags, const TagSequence& targetTags) const;
    virtual float getSingleUpdateScore ( 
          const Moses::WordsRange& sourceSegment, const Moses::WordsRange& targetSegment, 
          const TagSequence& newTargetTags) const;
    virtual float getFlipUpdateScore(const std::pair<Moses::WordsRange,Moses::WordsRange>& sourceSegments,
                                     const std::pair<Moses::WordsRange,Moses::WordsRange>& targetSegments) const
          {return 0;} //flipping can't change the verb difference
    
    virtual ~VerbDifferenceFeatureFunction() {}
    
  private:
    
  
};

}



