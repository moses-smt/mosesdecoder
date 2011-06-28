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

#pragma once


#include "FeatureFunction.h"
#include "Gibbler.h"
#include "Pos.h"

namespace Josiah {


/**
 * Features derived from projection of source pos tags onto target.
**/
class PosProjectionFeatureFunction: public FeatureFunction {
    public:
        PosProjectionFeatureFunction(const Sample& sample, Moses::FactorType sourceFactorType)
              : FeatureFunction(sample),
              m_sourceFactorType(sourceFactorType) {}
        /** Update the target words.*/
        virtual void updateTarget();
        virtual ~PosProjectionFeatureFunction() {}
    
    protected:
        /** All projected tags */
        const TagSequence& getCurrentTagProjection() const 
            { return m_tagProjection;}
        
        Moses::FactorType sourceFactorType() const {return m_sourceFactorType;}

    private:
       Moses::FactorType m_sourceFactorType;
       TagSequence m_tagProjection;
 
};

class PosProjectionBigramFeature : public Feature {
  public:
    PosProjectionBigramFeature(Moses::FactorType sourceFactorType,const std::string& tags);
    virtual FeatureFunctionHandle getFunction( const Sample& sample ) const;
    
  private:
    Moses::FactorType m_sourceFactorType;
    std::set<std::string> m_tags; //tags to be considered - empty means consider all tags
};

class PosProjectionBigramFeatureFunction : public PosProjectionFeatureFunction {
    public:

      PosProjectionBigramFeatureFunction(const Sample& sample, Moses::FactorType sourceFactorType,const std::set<std::string>& tags);

        /** Assign the total score of this feature on the current hypo */
        virtual void assignScore(FVector& scores);
    
        /** Score due to one segment */
        virtual void doSingleUpdate(const TranslationOption* option, const TargetGap& gap, FVector& scores);
        /** Score due to two segments. The left and right refer to the target positions.**/
        virtual void doContiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                              const TargetGap& gap, FVector& scores);
        virtual void doDiscontiguousPairedUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores);
    
        /** Score due to flip. Again, left and right refer to order on the <emph>target</emph> side. */
        virtual void doFlipUpdate(const TranslationOption* leftOption,const TranslationOption* rightOption, 
                                  const TargetGap& leftGap, const TargetGap& rightGap, FVector& scores) ;
    

    private:
        static const std::string ROOT;
      
        //Count the bigrams in the given tag sequence
        void countBigrams(const TagSequence& tagSequence, FVector& counts);
        const std::set<std::string>& m_tags; //tags to be considered - empty means consider all tags
};

}
